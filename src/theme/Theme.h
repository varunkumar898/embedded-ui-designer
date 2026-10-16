#pragma once

#include <QString>
#include <QColor>

namespace Theme {

// ── Backgrounds (Deep blacks with warm gray undertones, professional) ────────
inline constexpr const char* BgBase       = "#1A1A1A"; // Main window backdrop
inline constexpr const char* BgSurface    = "#252525"; // Menus, docks, panels
inline constexpr const char* BgRaised     = "#2D2D2D"; // Cards, inputs, toolbars
inline constexpr const char* BgOverlay    = "#383838"; // Hover states, popups
inline constexpr const char* BorderSubtle = "#3F3F3F"; // Subtle divider / border
inline constexpr const char* BorderStrong = "#4A4A4A"; // Prominent border

// ── Accent — Gray/White (used ONLY for selection, focus, primary actions) ────
inline constexpr const char* Accent        = "#FFFFFF"; // Primary action, focus (pure white)
inline constexpr const char* AccentHover   = "#E0E0E0"; // Hover state (light gray)
inline constexpr const char* AccentPressed = "#B0B0B0"; // Pressed state (darker gray)
inline constexpr const char* AccentSoft    = "#3A3A3A"; // Selected backgrounds, subtle tint
inline constexpr const char* AccentText    = "#FFFFFF"; // Text/icons on dark backgrounds

// ── Typography Colors ────────────────────────────────────────────────────────
inline constexpr const char* TextPrimary   = "#E8E8E8"; // High-emphasis text
inline constexpr const char* TextSecondary = "#B0B0B0"; // Medium-emphasis text
inline constexpr const char* TextMuted     = "#808080"; // Captions, placeholders
inline constexpr const char* TextDisabled  = "#606060"; // Disabled state text

// ── Semantic Feedback ────────────────────────────────────────────────────────
inline constexpr const char* Success       = "#4EC9B0"; // Teal/cyan — stands out on black
inline constexpr const char* Warning       = "#DCDCAA"; // Pale yellow — visible on dark
inline constexpr const char* Danger        = "#F48771"; // Salmon/red — warm, readable
inline constexpr const char* Info          = "#569CD6"; // Blue — matches editor style

// ── Semantic Tint Backgrounds (for badges, chips, statuses) ─────────────────
inline constexpr const char* SuccessBg     = "#1B2E2B"; // Dark teal tint
inline constexpr const char* WarningBg     = "#2E2C1A"; // Dark yellow tint
inline constexpr const char* DangerBg      = "#2E1F1C"; // Dark salmon tint
inline constexpr const char* InfoBg        = "#1A2638"; // Dark blue tint

// ── Live Test Active Color (Maintains compatibility) ─────────────────────────
inline constexpr const char* LiveTestActive = "#4EC9B0"; // Teal — consistent with success

// ── Icon States (Lucide SVGs are recolored from these tokens) ────────────────
inline constexpr const char* IconNormal   = TextSecondary; // #B0B0B0 idle
inline constexpr const char* IconHover    = TextPrimary;   // #E8E8E8 hover
inline constexpr const char* IconActive   = AccentText;    // #FFFFFF active/checked
inline constexpr const char* IconDisabled = TextDisabled;  // #606060 disabled (WCAG-exempt)

// ── Canvas Colors ────────────────────────────────────────────────────────────
inline constexpr const char* CanvasBg      = "#1F1F1F"; // Slightly lighter than bg-base

// ── Geometry & Sizing Tokens ─────────────────────────────────────────────────
inline constexpr int RadiusSm              = 4;
inline constexpr int RadiusMd              = 6;
inline constexpr int RadiusLg              = 8;
inline constexpr const char* RadiusSmStr   = "4px";
inline constexpr const char* RadiusMdStr   = "6px";
inline constexpr const char* RadiusLgStr   = "8px";

inline constexpr int SpaceXs               = 4;
inline constexpr int SpaceSm               = 8;
inline constexpr int SpaceMd               = 12;
inline constexpr int SpaceLg               = 16;
inline constexpr int SpaceXl               = 24;

inline constexpr int FontSizeSmall         = 11;
inline constexpr int FontSizeDefault       = 12;
inline constexpr int FontSizeHeading       = 13;

inline constexpr int FontWeightRegular     = 500;
inline constexpr int FontWeightEmphasized  = 600;

// ── QColor Conversion Accessors ──────────────────────────────────────────────
inline QColor cBgBase()       { return QColor(BgBase); }
inline QColor cBgSurface()    { return QColor(BgSurface); }
inline QColor cBgRaised()     { return QColor(BgRaised); }
inline QColor cBgOverlay()    { return QColor(BgOverlay); }
inline QColor cBorderSubtle() { return QColor(BorderSubtle); }
inline QColor cBorderStrong() { return QColor(BorderStrong); }

inline QColor cAccent()        { return QColor(Accent); }
inline QColor cAccentHover()   { return QColor(AccentHover); }
inline QColor cAccentPressed() { return QColor(AccentPressed); }
inline QColor cAccentSoft()    { return QColor(AccentSoft); }
inline QColor cAccentText()    { return QColor(AccentText); }

inline QColor cTextPrimary()   { return QColor(TextPrimary); }
inline QColor cTextSecondary() { return QColor(TextSecondary); }
inline QColor cTextMuted()     { return QColor(TextMuted); }
inline QColor cTextDisabled()  { return QColor(TextDisabled); }

inline QColor cSuccess()       { return QColor(Success); }
inline QColor cWarning()       { return QColor(Warning); }
inline QColor cDanger()        { return QColor(Danger); }
inline QColor cInfo()          { return QColor(Info); }

inline QColor cSuccessBg()     { return QColor(SuccessBg); }
inline QColor cWarningBg()     { return QColor(WarningBg); }
inline QColor cDangerBg()      { return QColor(DangerBg); }
inline QColor cInfoBg()        { return QColor(InfoBg); }

inline QColor cCanvasBg()      { return QColor(CanvasBg); }

inline QColor cIconNormal()    { return QColor(IconNormal); }
inline QColor cIconHover()     { return QColor(IconHover); }
inline QColor cIconActive()    { return QColor(IconActive); }
inline QColor cIconDisabled()  { return QColor(IconDisabled); }

// ── Master Stylesheet Generator ──────────────────────────────────────────────
QString stylesheet();

} // namespace Theme
