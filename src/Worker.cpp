// Copyright (c) 2011-2016 The Cryptonote developers
// Copyright (c) 2015-2016 XDN developers
// Copyright (c) 2017-2018, The karbo developers
// Copyright (c) 2018, The Qwertcoin developers
//
// This file is part of Qwertycoin.
//
// Qwertycoin is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Qwertycoin is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Qwertycoin. If not, see <http://www.gnu.org/licenses/>.

#include <QDebug>
#include <QReadWriteLock>
#include <QThread>

#include <crypto/hash.h>
#include <crypto/cn_slow_hash.hpp>
#include "NodeAdapter.h"
#include "CryptoNoteWrapper.h"
#include <CryptoNoteConfig.h>

#include "Worker.h"

namespace WalletGui {

Worker::Worker(QObject *parent, IWorkerObserver* _observer, Job& _currentJob, QReadWriteLock& _jobLock, std::atomic<quint32>& _nonce,
  std::atomic<quint32>& _hashCounter) : QObject(parent),
  m_observer(_observer), m_currentJob(_currentJob), m_jobLock(_jobLock), m_nonce(_nonce), m_hashCounter(_hashCounter), m_isStopped(true) {
  connect(this, &Worker::runSignal, this, &Worker::run, Qt::QueuedConnection);
}

void Worker::start() {
  m_isStopped = false;
  Q_EMIT runSignal();
}

void Worker::stop() {
  m_isStopped = true;
}

void Worker::run() {
  Job localJob;
  quint32 localNonce;
  Crypto::Hash hash;
  while (!m_isStopped) {
    {
      QReadLocker lock(&m_jobLock);
      if (m_currentJob.jobId.isEmpty()) {
        lock.unlock();
        QThread::msleep(100);
        continue;
      }

      if (localJob.jobId != m_currentJob.jobId) {
        localJob = m_currentJob;
      }
    }

    localNonce = ++m_nonce;
    localJob.blob.replace(39, sizeof(localNonce), reinterpret_cast<char*>(&localNonce), sizeof(localNonce));
    std::memset(&hash, 0, sizeof(hash));
  cn_pow_hash_v2 ctx;
    if (NodeAdapter::instance().getCurrentBlockMajorVersion() < CryptoNote::BLOCK_MAJOR_VERSION_4) {
      cn_pow_hash_v1 ctx_v1 = cn_pow_hash_v1::make_borrowed(ctx);
      ctx_v1.hash(localJob.blob.data(), localJob.blob.size(), hash.data);
    }
    else {
      ctx.hash(localJob.blob.data(), localJob.blob.size(), hash.data);
    }

    ++m_hashCounter;
    if (Q_UNLIKELY(((quint32*)&hash)[7] < localJob.target)) {
      m_observer->processShare(localJob.jobId, localNonce, QByteArray(reinterpret_cast<char*>(&hash), sizeof(hash)));
    }
  }
}

}
