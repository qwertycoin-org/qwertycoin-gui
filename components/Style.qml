pragma Singleton

import QtQuick 2.9

QtObject {
    property bool blackTheme: true

    // Qwertycoin typography. All fonts are bundled and loaded from Qt resources.
    property QtObject fontRegular: FontLoader { id: _fontRegular; source: "qrc:/fonts/Inter/Inter-Regular.otf" }
    property QtObject fontMedium: FontLoader { id: _fontMedium; source: "qrc:/fonts/Inter/Inter-SemiBold.otf" }
    property QtObject fontBold: FontLoader { id: _fontBold; source: "qrc:/fonts/Inter/Inter-SemiBold.otf" }
    property QtObject fontLight: FontLoader { id: _fontLight; source: "qrc:/fonts/Inter/Inter-Regular.otf" }
    property QtObject fontDisplay: FontLoader { id: _fontDisplay; source: "qrc:/fonts/Archivo/Archivo-Black.otf" }
    property QtObject fontMonoMedium: FontLoader { id: _fontMonoMedium; source: "qrc:/fonts/RobotoMono-Medium.ttf" }
    property QtObject fontMonoBold: FontLoader { id: _fontMonoBold; source: "qrc:/fonts/RobotoMono-Bold.ttf" }
    property QtObject fontMonoLight: FontLoader { id: _fontMonoLight; source: "qrc:/fonts/RobotoMono-Light.ttf" }
    property QtObject fontMonoRegular: FontLoader { id: _fontMonoRegular; source: "qrc:/fonts/RobotoMono-Regular.ttf" }
    property bool fontsReady: _fontRegular.status === FontLoader.Ready
                              && _fontMedium.status === FontLoader.Ready
                              && _fontDisplay.status === FontLoader.Ready
                              && _fontMonoRegular.status === FontLoader.Ready

    // Brand primitives shared by new and migrated components.
    readonly property color canvasColor: blackTheme ? "#141414" : "#F5F1E7"
    readonly property color cardColor: blackTheme ? "#1F1E1B" : "#FFFDF7"
    readonly property color raisedColor: blackTheme ? "#302D25" : "#FFF8E8"
    readonly property color textPrimaryColor: blackTheme ? "#F5F1E7" : "#141414"
    readonly property color textSecondaryColor: blackTheme ? "#C9C3B7" : "#615D55"
    readonly property color borderColor: blackTheme ? "#5A554B" : "#B8B0A3"
    readonly property color borderSubtleColor: blackTheme ? "#3C382F" : "#D9D1C4"
    readonly property color accentGold: "#FFAF00"
    readonly property color accentGoldHover: blackTheme ? "#FFC033" : "#F29F00"
    readonly property color accentGoldSoft: blackTheme ? "#4A3A16" : "#FFE7A3"
    readonly property color accentViolet: "#7952FF"
    readonly property color focusColor: "#7952FF"
    readonly property color successColor: blackTheme ? "#4FD69B" : "#0F7B47"
    readonly property color warningSurfaceColor: blackTheme ? "#3D3115" : "#FFF3C4"
    readonly property color errorSurfaceColor: blackTheme ? "#3A201E" : "#FFF1F0"
    readonly property color infoSurfaceColor: blackTheme ? "#29223D" : "#F1EDFF"
    readonly property color shadowColor: blackTheme ? "#000000" : "#4D463A"
    readonly property color hoverOverlayColor: blackTheme ? "#20FFFFFF" : "#10000000"
    readonly property color subtleOverlayColor: blackTheme ? "#0FFFFFFF" : "#08000000"
    readonly property color linkColor: blackTheme ? "#A98FFF" : "#5B35DB"
    readonly property color warningTextColor: blackTheme ? "#FFD166" : "#7A4A00"
    readonly property color incomingAmountColor: successColor
    readonly property color outgoingAmountColor: blackTheme ? "#FFB84D" : "#9C5C00"
    readonly property color actionTextColor: "#141414"

    readonly property int spaceXs: 4
    readonly property int spaceSm: 8
    readonly property int spaceMd: 12
    readonly property int spaceLg: 16
    readonly property int spaceXl: 24
    readonly property int space2Xl: 32
    readonly property int radiusSm: 4
    readonly property int radiusMd: 6
    readonly property int radiusLg: 8
    readonly property int controlHeight: 44
    readonly property int compactControlHeight: 36
    readonly property int contourWidth: 2

    // Compatibility aliases. Existing views can migrate without changing semantics.
    property string grey: borderColor
    property string orange: accentGold
    property string white: "#FFFFFF"
    property string green: successColor
    property string moneroGrey: raisedColor
    property string warningColor: accentGold

    property string defaultFontColor: blackTheme ? _b_defaultFontColor : _w_defaultFontColor
    property string dimmedFontColor: blackTheme ? _b_dimmedFontColor : _w_dimmedFontColor
    property string lightGreyFontColor: blackTheme ? _b_lightGreyFontColor : _w_lightGreyFontColor
    property string errorColor: blackTheme ? _b_errorColor : _w_errorColor
    property string textSelectionColor: blackTheme ? _b_textSelectionColor : _w_textSelectionColor
    property string textSelectedColor: blackTheme ? _b_textSelectedColor : _w_textSelectedColor

    property string inputBoxBackground: blackTheme ? _b_inputBoxBackground : _w_inputBoxBackground
    property string inputBoxBackgroundDisabled: blackTheme ? _b_inputBoxBackgroundDisabled : _w_inputBoxBackgroundDisabled
    property string inputBoxBackgroundError: blackTheme ? _b_inputBoxBackgroundError : _w_inputBoxBackgroundError
    property string inputBoxColor: blackTheme ? _b_inputBoxColor : _w_inputBoxColor
    property string legacy_placeholderFontColor: blackTheme ? _b_legacy_placeholderFontColor : _w_legacy_placeholderFontColor
    property string inputBorderColorActive: blackTheme ? _b_inputBorderColorActive : _w_inputBorderColorActive
    property string inputBorderColorInActive: blackTheme ? _b_inputBorderColorInActive : _w_inputBorderColorInActive
    property string inputBorderColorInvalid: blackTheme ? _b_inputBorderColorInvalid : _w_inputBorderColorInvalid

    property string buttonBackgroundColor: blackTheme ? _b_buttonBackgroundColor : _w_buttonBackgroundColor
    property string buttonBackgroundColorHover: blackTheme ? _b_buttonBackgroundColorHover : _w_buttonBackgroundColorHover
    property string buttonBackgroundColorDisabled: blackTheme ? _b_buttonBackgroundColorDisabled : _w_buttonBackgroundColorDisabled
    property string buttonBackgroundColorDisabledHover: blackTheme ? _b_buttonBackgroundColorDisabledHover : _w_buttonBackgroundColorDisabledHover
    property string buttonInlineBackgroundColor: blackTheme ? _b_buttonInlineBackgroundColor : _w_buttonInlineBackgroundColor
    property string buttonInlineBackgroundColorHover: blackTheme ? _b_buttonInlineBackgroundColorHover : _w_buttonInlineBackgroundColorHover
    property string buttonTextColor: blackTheme ? _b_buttonTextColor : _w_buttonTextColor
    property string buttonTextColorDisabled: blackTheme ? _b_buttonTextColorDisabled : _w_buttonTextColorDisabled
    property string buttonSecondaryBackgroundColor: blackTheme ? _b_buttonSecondaryBackgroundColor : _w_buttonSecondaryBackgroundColor
    property string buttonSecondaryBackgroundColorHover: blackTheme ? _b_buttonSecondaryBackgroundColorHover : _w_buttonSecondaryBackgroundColorHover
    property string buttonSecondaryTextColor: blackTheme ? _b_buttonSecondaryTextColor : _w_buttonSecondaryTextColor
    property string dividerColor: blackTheme ? _b_dividerColor : _w_dividerColor
    property real dividerOpacity: blackTheme ? _b_dividerOpacity : _w_dividerOpacity

    property string titleBarBackgroundGradientStart: blackTheme ? _b_titleBarBackgroundGradientStart : _w_titleBarBackgroundGradientStart
    property string titleBarBackgroundGradientStop: blackTheme ? _b_titleBarBackgroundGradientStop : _w_titleBarBackgroundGradientStop
    property string titleBarBackgroundBorderColor: blackTheme ? _b_titleBarBackgroundBorderColor : _w_titleBarBackgroundBorderColor
    property string titleBarLogoSource: blackTheme ? _b_titleBarLogoSource : _w_titleBarLogoSource
    property string titleBarMinimizeSource: blackTheme ? _b_titleBarMinimizeSource : _w_titleBarMinimizeSource
    property string titleBarFullscreenSource: blackTheme ? _b_titleBarFullscreenSource : _w_titleBarFullscreenSource
    property string titleBarCloseSource: blackTheme ? _b_titleBarCloseSource : _w_titleBarCloseSource
    property string titleBarButtonHoverColor: blackTheme ? _b_titleBarButtonHoverColor : _w_titleBarButtonHoverColor

    property string wizardBackgroundGradientStart: blackTheme ? _b_wizardBackgroundGradientStart : _w_wizardBackgroundGradientStart
    property string middlePanelBackgroundGradientStart: blackTheme ? _b_middlePanelBackgroundGradientStart : _w_middlePanelBackgroundGradientStart
    property string middlePanelBackgroundGradientStop: blackTheme ? _b_middlePanelBackgroundGradientStop : _w_middlePanelBackgroundGradientStop
    property string middlePanelBackgroundColor: blackTheme ? _b_middlePanelBackgroundColor : _w_middlePanelBackgroundColor
    property string menuButtonFallbackBackgroundColor: blackTheme ? _b_menuButtonFallbackBackgroundColor : _w_menuButtonFallbackBackgroundColor
    property string menuButtonGradientStart: blackTheme ? _b_menuButtonGradientStart : _w_menuButtonGradientStart
    property string menuButtonGradientStop: blackTheme ? _b_menuButtonGradientStop : _w_menuButtonGradientStop
    property string menuButtonTextColor: blackTheme ? _b_menuButtonTextColor : _w_menuButtonTextColor
    property string menuButtonImageRightColorActive: blackTheme ? _b_menuButtonImageRightColorActive : _w_menuButtonImageRightColorActive
    property string menuButtonImageRightColor: blackTheme ? _b_menuButtonImageRightColor : _w_menuButtonImageRightColor
    property string menuButtonImageRightSource: blackTheme ? _b_menuButtonImageRightSource : _w_menuButtonImageRightSource
    property string menuButtonImageDotArrowSource: blackTheme ? _b_menuButtonImageDotArrowSource : _w_menuButtonImageDotArrowSource
    property string inlineButtonTextColor: blackTheme ? _b_inlineButtonTextColor : _w_inlineButtonTextColor
    property string inlineButtonBorderColor: blackTheme ? _b_inlineButtonBorderColor : _w_inlineButtonBorderColor
    property string appWindowBackgroundColor: blackTheme ? _b_appWindowBackgroundColor : _w_appWindowBackgroundColor
    property string appWindowBorderColor: blackTheme ? _b_appWindowBorderColor : _w_appWindowBorderColor
    property bool progressBarProgressTextBold: blackTheme ? _b_progressBarProgressTextBold : _w_progressBarProgressTextBold
    property string progressBarBackgroundColor: blackTheme ? _b_progressBarBackgroundColor : _w_progressBarBackgroundColor
    property string leftPanelBackgroundGradientStart: blackTheme ? _b_leftPanelBackgroundGradientStart : _w_leftPanelBackgroundGradientStart
    property string leftPanelBackgroundGradientStop: blackTheme ? _b_leftPanelBackgroundGradientStop : _w_leftPanelBackgroundGradientStop
    property string historyHeaderTextColor: blackTheme ? _b_historyHeaderTextColor : _w_historyHeaderTextColor
    property var accountColors: blackTheme ? _b_accountColors : _w_accountColors

    property string _b_defaultFontColor: "#F5F1E7"
    property string _b_dimmedFontColor: "#C9C3B7"
    property string _b_lightGreyFontColor: "#D8D2C5"
    property string _b_errorColor: "#FF6B6B"
    property string _b_textSelectionColor: "#7952FF"
    property string _b_textSelectedColor: "#FFFFFF"
    property string _b_inputBoxBackground: "#1F1E1B"
    property string _b_inputBoxBackgroundDisabled: "#302D25"
    property string _b_inputBoxBackgroundError: "#3A201E"
    property string _b_inputBoxColor: "#F5F1E7"
    property string _b_legacy_placeholderFontColor: "#8E887D"
    property string _b_inputBorderColorActive: "#7952FF"
    property string _b_inputBorderColorInActive: "#5A554B"
    property string _b_inputBorderColorInvalid: "#FF6B6B"
    property string _b_buttonBackgroundColor: "#FFAF00"
    property string _b_buttonBackgroundColorHover: "#FFC033"
    property string _b_buttonBackgroundColorDisabled: "#6E685D"
    property string _b_buttonBackgroundColorDisabledHover: "#777168"
    property string _b_buttonInlineBackgroundColor: "#302D25"
    property string _b_buttonInlineBackgroundColorHover: "#3E3A30"
    property string _b_buttonTextColor: "#141414"
    property string _b_buttonTextColorDisabled: "#141414"
    property string _b_buttonSecondaryBackgroundColor: "#302D25"
    property string _b_buttonSecondaryBackgroundColorHover: "#3E3A30"
    property string _b_buttonSecondaryTextColor: "#F5F1E7"
    property string _b_dividerColor: "#6C6559"
    property real _b_dividerOpacity: 0.55
    property string _b_titleBarBackgroundGradientStart: "#1F1E1B"
    property string _b_titleBarBackgroundGradientStop: "#1F1E1B"
    property string _b_titleBarBackgroundBorderColor: "#3C382F"
    property string _b_titleBarLogoSource: "qrc:/images/brand/qwertycoin-wordmark-light.svg"
    property string _b_titleBarMinimizeSource: "qrc:/images/minimize.svg"
    property string _b_titleBarFullscreenSource: "qrc:/images/fullscreen.svg"
    property string _b_titleBarCloseSource: "qrc:/images/close.svg"
    property string _b_titleBarButtonHoverColor: "#302D25"
    property string _b_wizardBackgroundGradientStart: "#141414"
    property string _b_middlePanelBackgroundGradientStart: "#1F1E1B"
    property string _b_middlePanelBackgroundGradientStop: "#141414"
    property string _b_middlePanelBackgroundColor: "#1F1E1B"
    property string _b_menuButtonFallbackBackgroundColor: "#302D25"
    property string _b_menuButtonGradientStart: "#302D25"
    property string _b_menuButtonGradientStop: "#1F1E1B"
    property string _b_menuButtonTextColor: "#F5F1E7"
    property string _b_menuButtonImageRightColorActive: "#FFAF00"
    property string _b_menuButtonImageRightColor: "#C9C3B7"
    property string _b_menuButtonImageRightSource: "qrc:/images/right.svg"
    property string _b_menuButtonImageDotArrowSource: "qrc:/images/arrow-right-medium-white.png"
    property string _b_inlineButtonTextColor: "#F5F1E7"
    property string _b_inlineButtonBorderColor: "#5A554B"
    property string _b_appWindowBackgroundColor: "#141414"
    property string _b_appWindowBorderColor: "#3C382F"
    property bool _b_progressBarProgressTextBold: true
    property string _b_progressBarBackgroundColor: "#302D25"
    property string _b_leftPanelBackgroundGradientStart: "#1F1E1B"
    property string _b_leftPanelBackgroundGradientStop: "#141414"
    property string _b_historyHeaderTextColor: "#C9C3B7"
    property var _b_accountColors: ["#FFAF00", "#7952FF", "#0F7B47", "#C85D32", "#4185D8", "#9A5BC4", "#477D78", "#8A7760"]

    property string _w_defaultFontColor: "#141414"
    property string _w_dimmedFontColor: "#615D55"
    property string _w_lightGreyFontColor: "#777168"
    property string _w_errorColor: "#B42318"
    property string _w_textSelectionColor: "#7952FF"
    property string _w_textSelectedColor: "#FFFFFF"
    property string _w_inputBoxBackground: "#FFFDF7"
    property string _w_inputBoxBackgroundDisabled: "#E8E2D7"
    property string _w_inputBoxBackgroundError: "#FFF1F0"
    property string _w_inputBoxColor: "#141414"
    property string _w_legacy_placeholderFontColor: "#777168"
    property string _w_inputBorderColorActive: "#7952FF"
    property string _w_inputBorderColorInActive: "#B8B0A3"
    property string _w_inputBorderColorInvalid: "#B42318"
    property string _w_buttonBackgroundColor: "#FFAF00"
    property string _w_buttonBackgroundColorHover: "#F29F00"
    property string _w_buttonBackgroundColorDisabled: "#CEC6B7"
    property string _w_buttonBackgroundColorDisabledHover: "#D8D2C5"
    property string _w_buttonInlineBackgroundColor: "#FFF8E8"
    property string _w_buttonInlineBackgroundColorHover: "#FFE7A3"
    property string _w_buttonTextColor: "#141414"
    property string _w_buttonTextColorDisabled: "#615D55"
    property string _w_buttonSecondaryBackgroundColor: "#FFF8E8"
    property string _w_buttonSecondaryBackgroundColorHover: "#FFE7A3"
    property string _w_buttonSecondaryTextColor: "#141414"
    property string _w_dividerColor: "#B8B0A3"
    property real _w_dividerOpacity: 0.65
    property string _w_titleBarBackgroundGradientStart: "#FFFDF7"
    property string _w_titleBarBackgroundGradientStop: "#FFFDF7"
    property string _w_titleBarBackgroundBorderColor: "#D9D1C4"
    property string _w_titleBarLogoSource: "qrc:/images/brand/qwertycoin-wordmark-dark.svg"
    property string _w_titleBarMinimizeSource: "qrc:/images/themes/white/minimize.svg"
    property string _w_titleBarFullscreenSource: "qrc:/images/themes/white/fullscreen.svg"
    property string _w_titleBarCloseSource: "qrc:/images/themes/white/close.svg"
    property string _w_titleBarButtonHoverColor: "#FFF8E8"
    property string _w_wizardBackgroundGradientStart: "#F5F1E7"
    property string _w_middlePanelBackgroundGradientStart: "#FFF8E8"
    property string _w_middlePanelBackgroundGradientStop: "#F5F1E7"
    property string _w_middlePanelBackgroundColor: "#FFFDF7"
    property string _w_menuButtonFallbackBackgroundColor: "#FFF8E8"
    property string _w_menuButtonGradientStart: "#FFE7A3"
    property string _w_menuButtonGradientStop: "#FFF8E8"
    property string _w_menuButtonTextColor: "#141414"
    property string _w_menuButtonImageRightSource: "qrc:/images/right.svg"
    property string _w_menuButtonImageRightColorActive: "#7952FF"
    property string _w_menuButtonImageRightColor: "#615D55"
    property string _w_menuButtonImageDotArrowSource: "qrc:/images/arrow-right-medium-white.png"
    property string _w_inlineButtonTextColor: "#141414"
    property string _w_inlineButtonBorderColor: "#B8B0A3"
    property string _w_appWindowBackgroundColor: "#F5F1E7"
    property string _w_appWindowBorderColor: "#D9D1C4"
    property bool _w_progressBarProgressTextBold: false
    property string _w_progressBarBackgroundColor: "#E8E2D7"
    property string _w_leftPanelBackgroundGradientStart: "#FFFDF7"
    property string _w_leftPanelBackgroundGradientStop: "#F5F1E7"
    property string _w_historyHeaderTextColor: "#615D55"
    property var _w_accountColors: ["#D89000", "#7952FF", "#0F7B47", "#B94B22", "#2E6FB9", "#8642B3", "#2F6F6A", "#765E43"]
}
