// Copyright (c) 2026, The Qwertycoin Project
// SPDX-License-Identifier: BSD-3-Clause

.pragma library

function normalizeTransactionId(value) {
    var candidate = String(value || "").trim().toLowerCase();
    return /^[0-9a-f]{64}$/.test(candidate) ? candidate : "";
}

function copyObject(source) {
    var result = {};
    for (var key in source) {
        if (source.hasOwnProperty(key))
            result[key] = source[key];
    }
    return result;
}

function qwcAmountToAtomic(value) {
    var match = String(value || "").trim().match(/^(\d+)(?:\.(\d{0,8}))?$/);
    if (!match)
        return -1;
    var whole = Number(match[1]);
    var fraction = String(match[2] || "");
    while (fraction.length < 8)
        fraction += "0";
    var atomic = whole * 100000000 + Number(fraction || "0");
    if (!isFinite(atomic) || atomic < 0 || atomic > 9007199254740991 || Math.floor(atomic) !== atomic)
        return -1;
    return atomic;
}

function atomicToQwcAmount(atomic) {
    if (!isFinite(atomic) || atomic < 0 || atomic > 9007199254740991 || Math.floor(atomic) !== atomic)
        return "";
    var whole = Math.floor(atomic / 100000000);
    var fraction = String(atomic % 100000000);
    while (fraction.length < 8)
        fraction = "0" + fraction;
    return String(whole) + "." + fraction;
}

function normalizedGroups(groups) {
    var candidates = [];
    var claims = {};
    for (var groupIndex = 0; groupIndex < (groups || []).length; ++groupIndex) {
        var group = groups[groupIndex] || {};
        var storedIds = group.transactionIds || [];
        var transactionIds = [];
        var seen = {};
        var valid = storedIds.length > 0;
        for (var idIndex = 0; idIndex < storedIds.length; ++idIndex) {
            var transactionId = normalizeTransactionId(storedIds[idIndex]);
            if (!transactionId) {
                valid = false;
                continue;
            }
            claims[transactionId] = (claims[transactionId] || 0) + 1;
            if (seen[transactionId]) {
                valid = false;
                continue;
            }
            seen[transactionId] = true;
            transactionIds.push(transactionId);
        }
        if (valid) {
            candidates.push({
                group: group,
                transactionIds: transactionIds,
                valid: true
            });
        }
    }

    for (var candidateIndex = 0; candidateIndex < candidates.length; ++candidateIndex) {
        var candidate = candidates[candidateIndex];
        for (var transactionIndex = 0; transactionIndex < candidate.transactionIds.length; ++transactionIndex) {
            if (claims[candidate.transactionIds[transactionIndex]] !== 1) {
                candidate.valid = false;
                break;
            }
        }
    }
    return candidates;
}

function groupTransactions(transactions, groups) {
    var candidates = normalizedGroups(groups);
    var historyClaims = {};
    var historyCounts = {};
    var outputIndexes = {};
    var result = [];

    for (var transactionIndex = 0; transactionIndex < (transactions || []).length; ++transactionIndex) {
        var historyId = normalizeTransactionId(transactions[transactionIndex].hash);
        if (historyId)
            historyCounts[historyId] = (historyCounts[historyId] || 0) + 1;
    }

    for (var candidateIndex = 0; candidateIndex < candidates.length; ++candidateIndex) {
        var candidate = candidates[candidateIndex];
        if (!candidate.valid)
            continue;
        for (var idIndex = 0; idIndex < candidate.transactionIds.length; ++idIndex) {
            var transactionId = candidate.transactionIds[idIndex];
            if (historyCounts[transactionId] > 1) {
                candidate.valid = false;
                break;
            }
        }
        if (!candidate.valid)
            continue;
        for (var claimIndex = 0; claimIndex < candidate.transactionIds.length; ++claimIndex)
            historyClaims[candidate.transactionIds[claimIndex]] = candidateIndex;
    }

    for (var index = 0; index < (transactions || []).length; ++index) {
        var transaction = transactions[index];
        var normalizedHash = normalizeTransactionId(transaction.hash);
        var ownerIndex = historyClaims.hasOwnProperty(normalizedHash) ? historyClaims[normalizedHash] : -1;
        if (ownerIndex < 0) {
            var payment = copyObject(transaction);
            payment.isMessenger = false;
            payment.messengerMessageId = "";
            payment.messengerStatus = "";
            payment.messengerTransactionIdsText = "";
            payment.messengerTransactionCount = 0;
            payment.messengerMatchedTransactionCount = 0;
            result.push(payment);
            continue;
        }

        var owner = candidates[ownerIndex];
        var outputIndex = outputIndexes[ownerIndex];
        if (outputIndex === undefined) {
            var message = copyObject(transaction);
            var feeAtomic = qwcAmountToAtomic(transaction.fee);
            message.isMessenger = true;
            message.messengerMessageId = String(owner.group.messageId || "");
            message.messengerStatus = String(owner.group.status || "");
            message.messengerTransactionIdsText = normalizedHash;
            message.messengerTransactionCount = 1;
            message.messengerMatchedTransactionCount = 1;
            message.messengerFeeAtomic = feeAtomic;
            message.amount = 0;
            message.displayAmount = "";
            message.address = "";
            message.addressBookName = "";
            message.tx_note = "";
            message.fee = feeAtomic >= 0 ? atomicToQwcAmount(feeAtomic) : "";
            outputIndexes[ownerIndex] = result.length;
            result.push(message);
            continue;
        }

        var aggregate = result[outputIndex];
        var nextFeeAtomic = qwcAmountToAtomic(transaction.fee);
        aggregate.messengerMatchedTransactionCount += 1;
        aggregate.messengerTransactionCount += 1;
        aggregate.messengerTransactionIdsText += "\n" + normalizedHash;
        if (aggregate.messengerFeeAtomic >= 0 && nextFeeAtomic >= 0) {
            aggregate.messengerFeeAtomic += nextFeeAtomic;
            aggregate.fee = atomicToQwcAmount(aggregate.messengerFeeAtomic);
        } else {
            aggregate.messengerFeeAtomic = -1;
            aggregate.fee = "";
        }
        aggregate.isPending = aggregate.isPending || transaction.isPending;
        aggregate.isFailed = aggregate.isFailed || transaction.isFailed;
        aggregate.confirmations = Math.min(Number(aggregate.confirmations || 0), Number(transaction.confirmations || 0));
        if (Number(transaction.blockheight || 0) > Number(aggregate.blockheight || 0))
            aggregate.blockheight = transaction.blockheight;
        if (Number(transaction.timestamp || 0) > Number(aggregate.timestamp || 0)) {
            aggregate.timestamp = transaction.timestamp;
            aggregate.dateHuman = transaction.dateHuman;
            aggregate.dateTime = transaction.dateTime;
        }
    }
    return result;
}

function filterTransactions(transactions, filter) {
    var category = String(filter || "all");
    var result = [];
    for (var index = 0; index < (transactions || []).length; ++index) {
        var transaction = transactions[index];
        if (category === "payments" && transaction.isMessenger)
            continue;
        if (category === "messenger" && !transaction.isMessenger)
            continue;
        result.push(transaction);
    }
    return result;
}
