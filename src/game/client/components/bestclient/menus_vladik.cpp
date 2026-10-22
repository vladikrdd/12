/* Vladik tab: settings page with an MRX-style side menu. */
/* NOTE: generated from a table, the control list is at the bottom of RenderSettingsVladik. */
#include <engine/console.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/textrender.h>

#include <base/color.h>
#include <base/math.h>
#include <base/str.h>

#include <game/client/components/bestclient/ui_theme/style.h>
#include <game/client/components/menus.h>
#include <game/client/gameclient.h>
#include <game/client/lineinput.h>
#include <game/client/ui.h>
#include <game/client/ui_scrollregion.h>
#include <game/localization.h>

#include <algorithm>
#include <cstdlib>

namespace
{

struct SVladikCommand
{
	const char *m_pLabel;
	const char *m_pCommand;
};

constexpr int VLADIK_CATEGORY_COUNT = 7;

} // namespace

void CMenus::RenderSettingsVladik(CUIRect MainView)
{
	const float LineSize = 20.0f;
	const float MarginSmall = 5.0f;
	const float FontSize = 14.0f;
	const float HeadlineFontSize = 16.0f;
	const float SideWidth = 150.0f;

	// Persistent widget state. Every control has its own slot so ids never collide.
	static int s_Category = 0;
	static CButtonContainer s_aCategoryButtons[VLADIK_CATEGORY_COUNT];
	static CScrollRegion s_aScrollRegions[VLADIK_CATEGORY_COUNT];
	static CButtonContainer s_aKeyReaders[7];
	static CButtonContainer s_aKeyClears[7];
	static CButtonContainer s_aCmdButtons[20];
	static CButtonContainer s_aCycleButtons[6];
	static CButtonContainer s_aColorButtons[10];
	static CLineInput s_aStrInputs[6];
	static CLineInputBuffered<16> s_MaskInput;

	const char *apCategoryNames[VLADIK_CATEGORY_COUNT] = {BcLocalize("Aimbot"), BcLocalize("Avoid"), BcLocalize("TAS"), BcLocalize("Misc"), BcLocalize("Visuals"), BcLocalize("Fly Helper"), BcLocalize("Dummy")};
	s_Category = std::clamp(s_Category, 0, VLADIK_CATEGORY_COUNT - 1);

	// ---- layout: side menu + content panel
	CUIRect Side, Content;
	MainView.VSplitLeft(SideWidth, &Side, &Content);
	Content.VSplitLeft(10.0f, nullptr, &Content);

	BestClientUiTheme::DrawUiBlock(&Side, IGraphics::CORNER_ALL, 10.0f);
	Side.Margin(8.0f, &Side);

	CUIRect Title;
	Side.HSplitTop(28.0f, &Title, &Side);
	Ui()->DoLabel(&Title, BcLocalize("Vladik"), 20.0f, TEXTALIGN_MC);
	Side.HSplitTop(8.0f, nullptr, &Side);

	for(int i = 0; i < VLADIK_CATEGORY_COUNT; ++i)
	{
		CUIRect Button;
		Side.HSplitTop(28.0f, &Button, &Side);
		Side.HSplitTop(4.0f, nullptr, &Side);
		if(DoButton_Menu(&s_aCategoryButtons[i], apCategoryNames[i], s_Category == i ? 1 : 0, &Button))
			s_Category = i;
	}

	BestClientUiTheme::DrawUiBlock(&Content, IGraphics::CORNER_ALL, 10.0f);

	CUIRect View = Content;
	View.Margin(10.0f, &View);

	CScrollRegion &ScrollRegion = s_aScrollRegions[s_Category];
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ScrollUnit = 60.0f;
	ScrollParams.m_ScrollbarMargin = 5.0f;
	ScrollRegion.Begin(&View, &ScrollParams);
	View.VSplitRight(5.0f, &View, nullptr);
	View.VSplitLeft(5.0f, nullptr, &View);
	const float ScrollContentX = View.x;
	const float ScrollContentW = View.w;

	// ---- control helpers (all of them consume rows from View)
	auto VladikHeadline = [&](const char *pText, bool First) {
		if(!First)
			View.HSplitTop(12.0f, nullptr, &View);
		CUIRect Row, Line;
		View.HSplitTop(24.0f, &Row, &View);
		Ui()->DoLabel(&Row, pText, HeadlineFontSize, TEXTALIGN_ML);
		View.HSplitTop(1.0f, &Line, &View);
		Line.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.15f), IGraphics::CORNER_NONE, 0.0f);
		View.HSplitTop(6.0f, nullptr, &View);
	};

	auto VladikCheck = [&](int *pValue, const char *pLabel) {
		DoButton_CheckBoxAutoVMarginAndSet(pValue, pLabel, pValue, &View, LineSize);
		View.HSplitTop(MarginSmall, nullptr, &View);
	};

	auto VladikSlider = [&](int *pValue, const char *pLabel, int Min, int Max, const char *pSuffix) {
		CUIRect Row;
		View.HSplitTop(LineSize, &Row, &View);
		View.HSplitTop(MarginSmall, nullptr, &View);
		Ui()->DoScrollbarOption(pValue, pValue, &Row, pLabel, Min, Max, &CUi::ms_LinearScrollbarScale, 0u, pSuffix);
	};

	auto VladikCycle = [&](int *pValue, const char *pLabel, const char *const *ppOptions, int Count, CButtonContainer *pButton) {
		CUIRect Row, LabelRect, ButtonRect;
		View.HSplitTop(LineSize, &Row, &View);
		View.HSplitTop(MarginSmall, nullptr, &View);
		Row.VSplitMid(&LabelRect, &ButtonRect);
		Ui()->DoLabel(&LabelRect, pLabel, FontSize, TEXTALIGN_ML);
		*pValue = std::clamp(*pValue, 0, Count - 1);
		if(DoButton_Menu(pButton, ppOptions[*pValue], 0, &ButtonRect))
			*pValue = (*pValue + 1) % Count;
	};

	auto VladikColor = [&](unsigned int *pColor, unsigned int Default, const char *pLabel, CButtonContainer *pButton) {
		DoLine_ColorPicker(pButton, 25.0f, 13.0f, MarginSmall, &View, pLabel, pColor, color_cast<ColorRGBA>(ColorHSLA(Default, true)), false, nullptr, true);
	};

	auto VladikString = [&](char *pBuffer, size_t Size, const char *pLabel, CLineInput *pInput) {
		CUIRect Row, LabelRect, Field;
		View.HSplitTop(LineSize, &Row, &View);
		View.HSplitTop(MarginSmall, nullptr, &View);
		Row.VSplitLeft(std::min(150.0f, Row.w * 0.35f), &LabelRect, &Field);
		Ui()->DoLabel(&LabelRect, pLabel, FontSize, TEXTALIGN_ML);
		pInput->SetBuffer(pBuffer, Size);
		Ui()->DoClearableEditBox(pInput, &Field, 12.0f);
	};

	auto VladikMask = [&](int *pValue, const char *pLabel, int Max) {
		CUIRect Row, LabelRect, Field;
		View.HSplitTop(LineSize, &Row, &View);
		View.HSplitTop(MarginSmall, nullptr, &View);
		Row.VSplitLeft(std::min(150.0f, Row.w * 0.35f), &LabelRect, &Field);
		Ui()->DoLabel(&LabelRect, pLabel, FontSize, TEXTALIGN_ML);
		if(!s_MaskInput.IsActive())
		{
			char aBuf[32];
			str_format(aBuf, sizeof(aBuf), "0x%X", *pValue);
			s_MaskInput.Set(aBuf);
		}
		if(Ui()->DoEditBox(&s_MaskInput, &Field, 12.0f))
		{
			const long long Parsed = std::strtoll(s_MaskInput.GetString(), nullptr, 0);
			*pValue = (int)std::clamp<long long>(Parsed, 0, Max);
		}
	};

	auto VladikKey = [&](const char *pCommand, const char *pLabel, CButtonContainer *pReader, CButtonContainer *pClear) {
		DoLine_KeyReader(View, *pReader, *pClear, pLabel, pCommand);
	};

	auto VladikCommandRow = [&](const SVladikCommand *pCommands, int Count, CButtonContainer *pButtons) {
		const int Columns = 3;
		for(int i = 0; i < Count; i += Columns)
		{
			CUIRect Row;
			View.HSplitTop(LineSize, &Row, &View);
			View.HSplitTop(MarginSmall, nullptr, &View);
			for(int Column = 0; Column < Columns && i + Column < Count; ++Column)
			{
				CUIRect Cell;
				Row.VSplitLeft(Row.w / (Columns - Column), &Cell, &Row);
				Cell.VMargin(2.0f, &Cell);
				if(DoButton_Menu(&pButtons[i + Column], pCommands[i + Column].m_pLabel, 0, &Cell))
					Console()->ExecuteLine(pCommands[i + Column].m_pCommand, IConsole::CLIENT_ID_UNSPECIFIED);
			}
		}
	};

	auto VladikFileCommandRow = [&](const SVladikCommand *pCommands, int Count, const char *pFile, CButtonContainer *pButtons) {
		CUIRect Row;
		View.HSplitTop(LineSize, &Row, &View);
		View.HSplitTop(MarginSmall, nullptr, &View);
		for(int i = 0; i < Count; ++i)
		{
			CUIRect Cell;
			Row.VSplitLeft(Row.w / (Count - i), &Cell, &Row);
			Cell.VMargin(2.0f, &Cell);
			if(DoButton_Menu(&pButtons[i], pCommands[i].m_pLabel, 0, &Cell))
			{
				char aCommand[256];
				if(pFile[0] == '\0')
					str_copy(aCommand, pCommands[i].m_pCommand);
				else if(str_find(pFile, "\"") != nullptr || str_find(pFile, "\\") != nullptr)
					aCommand[0] = '\0'; // refuse file names that would break the command line
				else
					str_format(aCommand, sizeof(aCommand), "%s \"%s\"", pCommands[i].m_pCommand, pFile);
				if(aCommand[0] != '\0')
					Console()->ExecuteLine(aCommand, IConsole::CLIENT_ID_UNSPECIFIED);
			}
		}
	};

	auto VladikIntCommand = [&](const char *pLabel, const char *pCommand, int Value, CButtonContainer *pButton) {
		CUIRect Row, Cell;
		View.HSplitTop(LineSize, &Row, &View);
		View.HSplitTop(MarginSmall, nullptr, &View);
		Row.VSplitLeft(Row.w / 3.0f, &Cell, nullptr);
		Cell.VMargin(2.0f, &Cell);
		if(DoButton_Menu(pButton, pLabel, 0, &Cell))
		{
			char aCommand[128];
			str_format(aCommand, sizeof(aCommand), "%s %d", pCommand, Value);
			Console()->ExecuteLine(aCommand, IConsole::CLIENT_ID_UNSPECIFIED);
		}
	};

	// ---- controls of the selected category
	switch(s_Category)
	{
	case 0:
	{
		VladikHeadline(BcLocalize("Main settings"), true);
		VladikCheck(&g_Config.m_MrxAimbotEnable, BcLocalize("Enable Aimbot"));
		VladikSlider(&g_Config.m_MrxAimbotFov, BcLocalize("FOV"), 1, 360, "°");
		VladikCheck(&g_Config.m_MrxAimbotSilent, BcLocalize("Silent"));
		VladikCheck(&g_Config.m_MrxAimbotAutohook, BcLocalize("Auto Hook"));
		VladikKey("+mrx_aimbot_autohit", BcLocalize("Auto Hammer Hit (hold)"), &s_aKeyReaders[0], &s_aKeyClears[0]);
		VladikCheck(&g_Config.m_MrxAimbotEdge, BcLocalize("Edge Scan"));
		VladikSlider(&g_Config.m_MrxAimbotAccuracy, BcLocalize("Accuracy"), 1, 200, "%");
		VladikHeadline(BcLocalize("ESP"), false);
		VladikCheck(&g_Config.m_MrxEspEnable, BcLocalize("Enable ESP"));
		VladikCheck(&g_Config.m_MrxEspDrawFov, BcLocalize("Draw FOV"));
		VladikCheck(&g_Config.m_MrxEspFovAccurate, BcLocalize("FOV Accurate"));
		VladikCheck(&g_Config.m_MrxEspDrawBox, BcLocalize("Draw Box"));
		VladikCheck(&g_Config.m_MrxEspGlowPlayer, BcLocalize("Glow Player"));
		break;
	}
	case 1:
	{
		VladikHeadline(BcLocalize("Main settings"), true);
		VladikCheck(&g_Config.m_MrxAvoidEnable, BcLocalize("Enable AvoidFreeze"));
		VladikCheck(&g_Config.m_MrxAvoidHookAssist, BcLocalize("Hook Assist"));
		VladikCheck(&g_Config.m_MrxAvoidDirectionAssist, BcLocalize("Direction Assist"));
		VladikSlider(&g_Config.m_MrxAvoidCheckTicks, BcLocalize("Check Ticks"), 1, 20, "");
		VladikSlider(&g_Config.m_MrxAvoidCheckTicksHook, BcLocalize("Check Ticks Hook"), 1, 30, "");
		VladikCheck(&g_Config.m_MrxSmartHook, BcLocalize("Smart Hook"));
		VladikHeadline(BcLocalize("Tile types to avoid"), false);
		VladikCheck(&g_Config.m_MrxAvoidTileFreeze, BcLocalize("Avoid Freeze"));
		VladikCheck(&g_Config.m_MrxAvoidTileDeath, BcLocalize("Avoid Death"));
		VladikCheck(&g_Config.m_MrxAvoidTileTele, BcLocalize("Avoid Teleporter"));
		VladikCheck(&g_Config.m_MrxAvoidTileUnfreeze, BcLocalize("Avoid Unfreeze"));
		break;
	}
	case 2:
	{
		VladikHeadline(BcLocalize("Record and playback"), true);
		{
			static const SVladikCommand s_aCommands[] = {
				{"tas_record", "tas_record"},
				{"tas_stop", "tas_stop"},
				{"tas_play", "tas_play"},
				{"tas_play (from begin)", "tas_play 1"},
				{"tas_pause", "tas_pause"},
				{"tas_clear", "tas_clear"},
				{"tas_continue", "tas_continue"},
				{"tas_rewind", "tas_rewind"},
				{"tas_step", "tas_step"},
				{"tas_start_pos", "tas_start_pos"},
				{"tas_world_start", "tas_world_start"},
				{"tas_world_stop", "tas_world_stop"},
			};
			VladikCommandRow(s_aCommands, 12, &s_aCmdButtons[0]);
		}
		VladikString(g_Config.m_NggTasFile, sizeof(g_Config.m_NggTasFile), BcLocalize("TAS File"), &s_aStrInputs[0]);
		{
			static const SVladikCommand s_aCommands[] = {
				{"tas_load", "tas_load"},
				{"tas_save", "tas_save"},
			};
			VladikFileCommandRow(s_aCommands, 2, g_Config.m_NggTasFile, &s_aCmdButtons[12]);
		}
		VladikIntCommand(BcLocalize("Apply tas_tps"), "tas_tps", g_Config.m_NggTasTps, &s_aCmdButtons[14]);
		VladikKey("+mrxtaspanel", BcLocalize("TAS panel (hold)"), &s_aKeyReaders[1], &s_aKeyClears[1]);
		VladikHeadline(BcLocalize("MultiPair"), false);
		{
			static const SVladikCommand s_aCommands[] = {
				{"tas_multipair_start", "tas_multipair_start"},
				{"tas_multipair_switch", "tas_multipair_switch"},
				{"tas_multipair_stop", "tas_multipair_stop"},
			};
			VladikCommandRow(s_aCommands, 3, &s_aCmdButtons[15]);
		}
		VladikHeadline(BcLocalize("TAS settings"), false);
		VladikSlider(&g_Config.m_NggTasTps, BcLocalize("TAS TPS"), 1, 50, "");
		VladikCheck(&g_Config.m_NggTasRewind, BcLocalize("TAS Rewind"));
		VladikSlider(&g_Config.m_NggTasRewindTps, BcLocalize("TAS Rewind TPS"), 1, 50, "");
		VladikCheck(&g_Config.m_NggTasForward, BcLocalize("TAS Forward"));
		VladikSlider(&g_Config.m_NggTasDebug, BcLocalize("TAS Debug"), 0, 2, "");
		VladikCheck(&g_Config.m_NggTasPlayerUi, BcLocalize("TAS Player UI"));
		VladikSlider(&g_Config.m_NggTasStepFrames, BcLocalize("TAS Step Frames"), 1, 50, "");
		VladikCheck(&g_Config.m_NggTasSounds, BcLocalize("TAS Sounds"));
		VladikCheck(&g_Config.m_NggTasEffects, BcLocalize("TAS Effects"));
		VladikCheck(&g_Config.m_NggTasHud, BcLocalize("TAS HUD"));
		VladikCheck(&g_Config.m_NggTasPath, BcLocalize("TAS Path"));
		VladikCheck(&g_Config.m_NggTasPathLimited, BcLocalize("TAS Path Limited"));
		VladikSlider(&g_Config.m_NggTasPathLength, BcLocalize("TAS Path Length"), 1, 1000, "");
		VladikCheck(&g_Config.m_NggTasPanelClickOnly, BcLocalize("TAS Panel Click Only"));
		VladikCheck(&g_Config.m_NggTasAutosave, BcLocalize("TAS Auto Save"));
		VladikSlider(&g_Config.m_NggTasAutosaveInterval, BcLocalize("TAS Auto Save Interval"), 0, 5000, "");
		VladikCheck(&g_Config.m_MrxTasAutoMargin, BcLocalize("TAS Auto Margin"));
		VladikSlider(&g_Config.m_MrxTasMargin, BcLocalize("TAS Margin"), 0, 300, "");
		break;
	}
	case 3:
	{
		VladikHeadline(BcLocalize("Misc settings"), true);
		VladikCheck(&g_Config.m_MrxAutoaled, BcLocalize("Auto Aled"));
		VladikCheck(&g_Config.m_MrxDummyaled, BcLocalize("Dummy Aled"));
		VladikCheck(&g_Config.m_MrxFouraled, BcLocalize("Four Aled"));
		VladikCheck(&g_Config.m_MrxDummyHookcursor, BcLocalize("Dummy Hook Cursor"));
		VladikCheck(&g_Config.m_MrxAutohookdummy, BcLocalize("Auto Hook Dummy"));
		VladikCheck(&g_Config.m_MrxNullMovement, BcLocalize("Null Movement"));
		VladikCheck(&g_Config.m_MrxFaststop, BcLocalize("FastStop"));
		VladikCheck(&g_Config.m_MrxReverseDummyCopyMoves, BcLocalize("Reverse Copy"));
		VladikCheck(&g_Config.m_MrxHideChatEmote, BcLocalize("Hide Chat Emote"));
		VladikCheck(&g_Config.m_MrxDummyFreezeOutline, BcLocalize("Dummy Freeze Outline"));
		VladikCheck(&g_Config.m_MrxScoreboard, BcLocalize("MRX Scoreboard"));
		VladikHeadline(BcLocalize("Auto Dummy Connect"), false);
		VladikCheck(&g_Config.m_MrxAutoDummyConnect, BcLocalize("Auto Dummy Connect"));
		VladikSlider(&g_Config.m_MrxAutoDummyConnectCheckInterval, BcLocalize("Interval"), 500, 10000, " ms");
		VladikCheck(&g_Config.m_MrxNoSwitchDummyOnConnected, BcLocalize("Don't Switch on Connect"));
		VladikCheck(&g_Config.m_MrxAutoDummyConnectDummy1, BcLocalize("Auto Connect Dummy"));
		VladikCheck(&g_Config.m_MrxAutoDummyConnectDummy2, BcLocalize("Auto Connect Dummy2"));
		VladikCheck(&g_Config.m_MrxAutoDummyConnectDummy3, BcLocalize("Auto Connect Dummy3"));
		VladikHeadline(BcLocalize("Balance Bot"), false);
		VladikKey("toggle mrx_balancebot 0 1", BcLocalize("Balance Bot Key"), &s_aKeyReaders[2], &s_aKeyClears[2]);
		{ static const char *s_apOpt0[] = {BcLocalize("Below"), BcLocalize("Above"), BcLocalize("Both")}; VladikCycle(&g_Config.m_MrxBalancebotVFilter, BcLocalize("Mode"), s_apOpt0, 3, &s_aCycleButtons[0]); }
		{ static const char *s_apOpt1[] = {BcLocalize("Normal"), BcLocalize("Precise"), BcLocalize("Legit")}; VladikCycle(&g_Config.m_MrxBalancebotPrecise, BcLocalize("Precise"), s_apOpt1, 3, &s_aCycleButtons[1]); }
		VladikHeadline(BcLocalize("Auto Team"), false);
		VladikCheck(&g_Config.m_MrxAutoTeamEnable, BcLocalize("Auto Team"));
		VladikSlider(&g_Config.m_MrxAutoTeamNumber, BcLocalize("Team"), 1, 63, "");
		VladikCheck(&g_Config.m_MrxAutoTeamLock, BcLocalize("Lock Team"));
		VladikCheck(&g_Config.m_MrxAutoTeamInviteDummy1, BcLocalize("Invite Dummy"));
		VladikCheck(&g_Config.m_MrxAutoTeamInviteDummy2, BcLocalize("Invite Dummy2"));
		VladikCheck(&g_Config.m_MrxAutoTeamInviteDummy3, BcLocalize("Invite Dummy3"));
		break;
	}
	case 4:
	{
		VladikHeadline(BcLocalize("Overlay Text"), true);
		VladikCheck(&g_Config.m_MrxOverlaytext, BcLocalize("Overlay Text"));
		VladikString(g_Config.m_MrxText, sizeof(g_Config.m_MrxText), BcLocalize("Text 0"), &s_aStrInputs[1]);
		VladikString(g_Config.m_MrxText1, sizeof(g_Config.m_MrxText1), BcLocalize("Text 1"), &s_aStrInputs[2]);
		VladikString(g_Config.m_MrxText2, sizeof(g_Config.m_MrxText2), BcLocalize("Text 2"), &s_aStrInputs[3]);
		VladikString(g_Config.m_MrxText3, sizeof(g_Config.m_MrxText3), BcLocalize("Text 3"), &s_aStrInputs[4]);
		VladikString(g_Config.m_MrxText4, sizeof(g_Config.m_MrxText4), BcLocalize("Text 4"), &s_aStrInputs[5]);
		VladikSlider(&g_Config.m_MrxTextX, BcLocalize("Text Position X"), 0, 10000, "");
		VladikSlider(&g_Config.m_MrxTextY, BcLocalize("Text Position Y"), 0, 10000, "");
		VladikColor(&g_Config.m_MrxTextColor, DefaultConfig::MrxTextColor, BcLocalize("Text Color"), &s_aColorButtons[0]);
		VladikHeadline(BcLocalize("ArrayList"), false);
		VladikCheck(&g_Config.m_MrxArraylist, BcLocalize("ArrayList"));
		VladikCheck(&g_Config.m_MrxArraylistGradient, BcLocalize("ArrayList Gradient"));
		VladikCheck(&g_Config.m_MrxArraylistLine, BcLocalize("ArrayList Line"));
		VladikMask(&g_Config.m_MrxArraylistMask, BcLocalize("ArrayList Mask"), 0x3FFFFF);
		VladikSlider(&g_Config.m_MrxArraylistX, BcLocalize("ArrayList Position X"), 0, 20000, "");
		VladikSlider(&g_Config.m_MrxArraylistY, BcLocalize("ArrayList Position Y"), 0, 20000, "");
		VladikHeadline(BcLocalize("Misc Visuals"), false);
		VladikCheck(&g_Config.m_MrxOverlaySamex, BcLocalize("Text output when sameX"));
		VladikCheck(&g_Config.m_MrxFrozenKatana, BcLocalize("Show Katan Frozen"));
		VladikCheck(&g_Config.m_MrxCenterEnable, BcLocalize("Crosshair in the center"));
		VladikSlider(&g_Config.m_MrxCenterWidth, BcLocalize("Thickness Crosshair"), 1, 10, "");
		VladikColor(&g_Config.m_MrxCenterColor, DefaultConfig::MrxCenterColor, BcLocalize("Color Crosshair"), &s_aColorButtons[1]);
		VladikCheck(&g_Config.m_MrxRenderLaser, BcLocalize("Render Laser"));
		VladikCheck(&g_Config.m_MrxAnimChat, BcLocalize("Animated chat"));
		VladikSlider(&g_Config.m_MrxFpsMultiplier, BcLocalize("FPS Multiplier"), 1, 1000, "");
		VladikSlider(&g_Config.m_MrxCursorScale, BcLocalize("Cursor Scale"), 1, 500, "");
		VladikCheck(&g_Config.m_MrxFreezeTilesTint, BcLocalize("Tint freeze tiles"));
		VladikSlider(&g_Config.m_MrxFreezeTilesAlphaMul, BcLocalize("Freeze tint intensity"), 0, 200, "%");
		VladikSlider(&g_Config.m_MrxFreezeTilesBrightness, BcLocalize("Freeze tint brightness"), 50, 200, "%");
		VladikColor(&g_Config.m_MrxFreezeTilesColor, DefaultConfig::MrxFreezeTilesColor, BcLocalize("Freeze tiles color"), &s_aColorButtons[2]);
		VladikHeadline(BcLocalize("Custom MRX Skins"), false);
		VladikCheck(&g_Config.m_MrxVisualSkinOverride, BcLocalize("Visual skin override"));
		{ static const char *s_apOpt2[] = {BcLocalize("Player"), BcLocalize("Dummy")}; VladikCycle(&g_Config.m_MrxVisualSkinSource, BcLocalize("Source"), s_apOpt2, 2, &s_aCycleButtons[2]); }
		{ static const char *s_apOpt3[] = {BcLocalize("Me"), BcLocalize("Dummies"), BcLocalize("Others"), BcLocalize("All")}; VladikCycle(&g_Config.m_MrxVisualSkinTarget, BcLocalize("Apply to"), s_apOpt3, 4, &s_aCycleButtons[3]); }
		VladikHeadline(BcLocalize("Background Lines Menu"), false);
		VladikCheck(&g_Config.m_MrxBgLinesEnable, BcLocalize("Enable background lines"));
		VladikCheck(&g_Config.m_MrxBgLinesDots, BcLocalize("Draw dots"));
		VladikSlider(&g_Config.m_MrxBgLinesPoints, BcLocalize("Points"), 3, 128, "");
		VladikSlider(&g_Config.m_MrxBgLinesDistance, BcLocalize("Distance"), 1, 500, "");
		VladikSlider(&g_Config.m_MrxBgLinesSpeed, BcLocalize("Speed"), 1, 200, "");
		VladikSlider(&g_Config.m_MrxBgLinesRadius, BcLocalize("Radius"), 1, 20, "");
		VladikSlider(&g_Config.m_MrxBgLinesAlpha, BcLocalize("Alpha"), 0, 100, "%");
		VladikColor(&g_Config.m_MrxBgLinesColor, DefaultConfig::MrxBgLinesColor, BcLocalize("Color"), &s_aColorButtons[3]);
		VladikHeadline(BcLocalize("Predicted Trajectory"), false);
		VladikCheck(&g_Config.m_MrxPredictTraj, BcLocalize("Enable"));
		VladikCheck(&g_Config.m_MrxPredictTrajDummy, BcLocalize("Enable for Dummy"));
		VladikSlider(&g_Config.m_MrxPredictTrajTicks, BcLocalize("Ticks"), 0, 200, "");
		VladikSlider(&g_Config.m_MrxPredictTrajThickness, BcLocalize("Path thickness"), 1, 10, "");
		VladikCheck(&g_Config.m_MrxPredictTrajOtherPlayers, BcLocalize("Predict players"));
		{ static const char *s_apOpt4[] = {BcLocalize("Solid"), BcLocalize("Dashed"), BcLocalize("Dotted")}; VladikCycle(&g_Config.m_MrxPredictTrajMode, BcLocalize("Mode"), s_apOpt4, 3, &s_aCycleButtons[4]); }
		VladikColor(&g_Config.m_MrxPredictTrajColor, DefaultConfig::MrxPredictTrajColor, BcLocalize("Base color"), &s_aColorButtons[4]);
		VladikColor(&g_Config.m_MrxPredictTrajColorFreeze, DefaultConfig::MrxPredictTrajColorFreeze, BcLocalize("Freeze color"), &s_aColorButtons[5]);
		VladikColor(&g_Config.m_MrxPredictTrajColorDummy, DefaultConfig::MrxPredictTrajColorDummy, BcLocalize("Dummy color"), &s_aColorButtons[6]);
		VladikHeadline(BcLocalize("Swap Timer"), false);
		VladikCheck(&g_Config.m_MrxSwapTimer, BcLocalize("Swap Timer"));
		VladikSlider(&g_Config.m_MrxSwapTimerX, BcLocalize("Position X"), 0, 20000, "");
		VladikSlider(&g_Config.m_MrxSwapTimerY, BcLocalize("Position Y"), 0, 20000, "");
		VladikColor(&g_Config.m_MrxSwapTimerColor, DefaultConfig::MrxSwapTimerColor, BcLocalize("Text color"), &s_aColorButtons[7]);
		VladikColor(&g_Config.m_MrxSwapTimerBgColor, DefaultConfig::MrxSwapTimerBgColor, BcLocalize("BG color"), &s_aColorButtons[8]);
		VladikColor(&g_Config.m_MrxSwapTimerInitiatorColor, DefaultConfig::MrxSwapTimerInitiatorColor, BcLocalize("Initiator color"), &s_aColorButtons[9]);
		break;
	}
	case 5:
	{
		VladikHeadline(BcLocalize("AutoPseudo"), true);
		VladikCheck(&g_Config.m_MrxDummyPseudo, BcLocalize("Autopseudo"));
		VladikSlider(&g_Config.m_MrxTargetHit, BcLocalize("TimeTargetHit"), 1, 15, "");
		VladikCheck(&g_Config.m_MrxCopy, BcLocalize("HammerMove"));
		VladikHeadline(BcLocalize("DuoFly"), false);
		VladikCheck(&g_Config.m_MrxDuoPseudo, BcLocalize("DuoPseudo"));
		VladikCheck(&g_Config.m_MrxDuoPseudoTarget, BcLocalize("DuoPseudo Target"));
		VladikCheck(&g_Config.m_MrxDuoPseudoAutoAim, BcLocalize("DuoPseudo AutoAim"));
		VladikCheck(&g_Config.m_MrxDuoPseudoFire, BcLocalize("DuoPseudo Fire"));
		VladikCheck(&g_Config.m_MrxDuoHookPredict, BcLocalize("Hook likes Pseudofly"));
		VladikSlider(&g_Config.m_MrxDuoPredictHookTicks, BcLocalize("DuoPredictHook Ticks"), 1, 30, "");
		VladikSlider(&g_Config.m_MrxDuoPredictHookDistance, BcLocalize("DuoPredictHook Distance"), 52, 200, "");
		VladikHeadline(BcLocalize("4tee"), false);
		VladikCheck(&g_Config.m_MrxAuto5fly, BcLocalize("PentaFly"));
		VladikSlider(&g_Config.m_MrxAuto5flyDistance, BcLocalize("PentaFly Distance"), 10, 380, "");
		VladikHeadline(BcLocalize("HookFly"), false);
		VladikSlider(&g_Config.m_MrxHookflyAutoDistance, BcLocalize("AutoHookFly Distance"), 0, 250, "");
		VladikCheck(&g_Config.m_MrxHookflyAuto, BcLocalize("AutoHookFly"));
		VladikKey("+toggle mrx_hookfly_manual 1 0", BcLocalize("ManualHookFly key"), &s_aKeyReaders[3], &s_aKeyClears[3]);
		VladikCheck(&g_Config.m_MrxDummyHookPredict, BcLocalize("Dummy hook likes Pseudofly"));
		VladikHeadline(BcLocalize("Movefly settings"), false);
		VladikCheck(&g_Config.m_MrxMoveflyBotDirect, BcLocalize("Movefly [D]copy dir"));
		VladikCheck(&g_Config.m_MrxMoveflyPlayer, BcLocalize("MoveFlyPlayerStopDir"));
		VladikSlider(&g_Config.m_MrxMoveflyBotDistance, BcLocalize("Movefly Distance"), 0, 50, "");
		VladikSlider(&g_Config.m_MrxMoveflyOverdistance, BcLocalize("Movefly OverDistance"), 0, 63, "");
		VladikKey("toggle mrx_movefly 0 1; dummy_reset", BcLocalize("Movefly key"), &s_aKeyReaders[4], &s_aKeyClears[4]);
		VladikHeadline(BcLocalize("AutoTripleFly"), false);
		VladikCheck(&g_Config.m_MrxAutotriplefly, BcLocalize("AutoTripleFly (hold LMB)"));
		{ static const char *s_apOpt5[] = {BcLocalize("AutoTripleControl"), BcLocalize("AutoTripleDefaultCopy")}; VladikCycle(&g_Config.m_MrxAutotripleflyMode, BcLocalize("Mode"), s_apOpt5, 2, &s_aCycleButtons[5]); }
		break;
	}
	case 6:
	{
		VladikHeadline(BcLocalize("Copy"), true);
		VladikCheck(&g_Config.m_MrxDummyCopyDirection, BcLocalize("Direction"));
		VladikCheck(&g_Config.m_MrxDummyCopyHook, BcLocalize("Hook"));
		VladikCheck(&g_Config.m_MrxDummyCopyAim, BcLocalize("Aim"));
		VladikCheck(&g_Config.m_MrxDummyCopyJump, BcLocalize("Jump"));
		VladikCheck(&g_Config.m_MrxDummyCopyFire, BcLocalize("Fire"));
		VladikCheck(&g_Config.m_MrxDummyCopyWeapon, BcLocalize("Weapon"));
		VladikCheck(&g_Config.m_MrxDummyCopyPlayerFlags, BcLocalize("Flags"));
		VladikHeadline(BcLocalize("4tee"), false);
		VladikCheck(&g_Config.m_MrxDummyDynamicPair, BcLocalize("Dynamic pair"));
		VladikKey("toggle mrx_dummy_copy_moves_all 1 0", BcLocalize("All Dummy Copy Moves"), &s_aKeyReaders[5], &s_aKeyClears[5]);
		VladikKey("toggle mrx_switch_pair 0 1", BcLocalize("SwitchPair"), &s_aKeyReaders[6], &s_aKeyClears[6]);
		{
			static const SVladikCommand s_aCommands[] = {
				{"Switch dummy to cursor", "switch_dummy_to_cursor"},
				{"Switch dummy in pair", "switch_dummy_to_pair"},
			};
			VladikCommandRow(s_aCommands, 2, &s_aCmdButtons[18]);
		}
		VladikHeadline(BcLocalize("Dummy Replay"), false);
		VladikCheck(&g_Config.m_MrxDummyReplay, BcLocalize("Enable Replay"));
		VladikSlider(&g_Config.m_MrxDummyReplayDelay, BcLocalize("Delay (ticks)"), 1, 600, "");
		VladikCheck(&g_Config.m_MrxDummyReplayAll, BcLocalize("All Dummies"));
		VladikSlider(&g_Config.m_MrxDummyReplayInterDummyDelay, BcLocalize("Inter-Dummy Delay"), 0, 600, "");
		break;
	}
	default:
		break;
	}

	CUIRect ScrollContentRect;
	ScrollContentRect.x = ScrollContentX;
	ScrollContentRect.y = View.y + MarginSmall * 2.0f;
	ScrollContentRect.w = ScrollContentW;
	ScrollContentRect.h = 0.0f;
	ScrollRegion.AddRect(ScrollContentRect);
	ScrollRegion.End();
}
