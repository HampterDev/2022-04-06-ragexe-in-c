#include "../type_query_fixture.h"
#include <stdint.h>

struct ReferenceType { const void *ops; void *cache; char name[256]; };
static const struct ReferenceType type_CRes = {0, 0, ".?AVCRes@@"};
static const struct ReferenceType type_CCharacterNpc = {0, 0, ".?AVCCharacterNpc@@"};
static const struct ReferenceType type_CNpc = {0, 0, ".?AVCNpc@@"};
static const struct ReferenceType type_CGameActor = {0, 0, ".?AVCGameActor@@"};
static const struct ReferenceType type_CAbleToMakeEffect = {0, 0, ".?AVCAbleToMakeEffect@@"};
static const struct ReferenceType type_CRenderObject = {0, 0, ".?AVCRenderObject@@"};
static const struct ReferenceType type_CGameObject = {0, 0, ".?AVCGameObject@@"};
static const struct ReferenceType type_UIEditCtrl = {0, 0, ".?AVUIEditCtrl@@"};
static const struct ReferenceType type_UIWindow = {0, 0, ".?AVUIWindow@@"};
static const struct ReferenceType type_UIRPData = {0, 0, ".?AVUIRPData@@"};
static const struct ReferenceType type_UIBitmapEditCtrl = {0, 0, ".?AVUIBitmapEditCtrl@@"};
static const struct ReferenceType type_UIFrameWnd = {0, 0, ".?AVUIFrameWnd@@"};
static const struct ReferenceType type_UIListBox_NaviSearch = {0, 0, ".?AVUIListBox_NaviSearch@@"};
static const struct ReferenceType type_UIListBox = {0, 0, ".?AVUIListBox@@"};
static const struct ReferenceType type_UINavigationroadiconWnd = {0, 0, ".?AVUINavigationroadiconWnd@@"};
static const struct ReferenceType type_UINavigationRuideWnd = {0, 0, ".?AVUINavigationRuideWnd@@"};
static const struct ReferenceType type_UIPrivateAirplaneWnd = {0, 0, ".?AVUIPrivateAirplaneWnd@@"};
static const struct ReferenceType type_UINavigationV4Wnd = {0, 0, ".?AVUINavigationV4Wnd@@"};
static const struct ReferenceType type_CRollingCutterRagEffect = {0, 0, ".?AVCRollingCutterRagEffect@@"};
static const struct ReferenceType type_CRagEffect = {0, 0, ".?AVCRagEffect@@"};
static const struct ReferenceType type_CBitmapRes = {0, 0, ".?AVCBitmapRes@@"};
static const struct ReferenceType type_CPaletteRes = {0, 0, ".?AVCPaletteRes@@"};
static const struct ReferenceType type_CEmblemRes = {0, 0, ".?AVCEmblemRes@@"};
static const struct ReferenceType type_CGifRes = {0, 0, ".?AVCGifRes@@"};
static const struct ReferenceType type_UIEditCtrl2 = {0, 0, ".?AVUIEditCtrl2@@"};
static const struct ReferenceType type_UIEditCtrl4 = {0, 0, ".?AVUIEditCtrl4@@"};
static const struct ReferenceType type_UIEditCtrl5 = {0, 0, ".?AVUIEditCtrl5@@"};
static const struct ReferenceType type_UIEditCtrl3 = {0, 0, ".?AVUIEditCtrl3@@"};
static const struct ReferenceType type_UIChatEditCtrl = {0, 0, ".?AVUIChatEditCtrl@@"};
static const struct ReferenceType type_UIWhisperEditCtrl = {0, 0, ".?AVUIWhisperEditCtrl@@"};
static const struct ReferenceType type_UIEditCtrl_OldList = {0, 0, ".?AVUIEditCtrl_OldList@@"};
static const struct ReferenceType type_UIStaticText = {0, 0, ".?AVUIStaticText@@"};
static const struct ReferenceType type_UIStaticText3 = {0, 0, ".?AVUIStaticText3@@"};
static const struct ReferenceType type_UIBalloonText = {0, 0, ".?AVUIBalloonText@@"};
static const struct ReferenceType type_UINameBalloonText = {0, 0, ".?AVUINameBalloonText@@"};
static const struct ReferenceType type_UITransBalloonText = {0, 0, ".?AVUITransBalloonText@@"};
static const struct ReferenceType type_UIAchBalloonText = {0, 0, ".?AVUIAchBalloonText@@"};
static const struct ReferenceType type_UIButton = {0, 0, ".?AVUIButton@@"};
static const struct ReferenceType type_UINoneVisibleButton = {0, 0, ".?AVUINoneVisibleButton@@"};
static const struct ReferenceType type_UIColorButton = {0, 0, ".?AVUIColorButton@@"};
static const struct ReferenceType type_UIBitmapButton = {0, 0, ".?AVUIBitmapButton@@"};
static const struct ReferenceType type_UIBasicButton = {0, 0, ".?AVUIBasicButton@@"};
static const struct ReferenceType type_UIBmpTextButton = {0, 0, ".?AVUIBmpTextButton@@"};
static const struct ReferenceType type_UIBmpTextButton2 = {0, 0, ".?AVUIBmpTextButton2@@"};
static const struct ReferenceType type_UITextureButton = {0, 0, ".?AVUITextureButton@@"};
static const struct ReferenceType type_UISpriteButton = {0, 0, ".?AVUISpriteButton@@"};
static const struct ReferenceType type_UIHeadsprButton = {0, 0, ".?AVUIHeadsprButton@@"};
static const struct ReferenceType type_UICheckBox = {0, 0, ".?AVUICheckBox@@"};
static const struct ReferenceType type_UIStaticBitmap = {0, 0, ".?AVUIStaticBitmap@@"};
static const struct ReferenceType type_UISlotBitmap = {0, 0, ".?AVUISlotBitmap@@"};
static const struct ReferenceType type_UIRadioBtn = {0, 0, ".?AVUIRadioBtn@@"};
static const struct ReferenceType type_UIResizer = {0, 0, ".?AVUIResizer@@"};
static const struct ReferenceType type_UITab = {0, 0, ".?AVUITab@@"};
static const struct ReferenceType type_UIBitmapTab = {0, 0, ".?AVUIBitmapTab@@"};
static const struct ReferenceType type_UIChatRoomTitle = {0, 0, ".?AVUIChatRoomTitle@@"};
static const struct ReferenceType type_UIMerchantShopTitle = {0, 0, ".?AVUIMerchantShopTitle@@"};
static const struct ReferenceType type_UIEmblemWnd = {0, 0, ".?AVUIEmblemWnd@@"};
static const struct ReferenceType type_UIWordDisplayWnd = {0, 0, ".?AVUIWordDisplayWnd@@"};
static const struct ReferenceType type_UIScrollBar = {0, 0, ".?AVUIScrollBar@@"};
static const struct ReferenceType type_UIScrollBar2 = {0, 0, ".?AVUIScrollBar2@@"};
static const struct ReferenceType type_UIScrollBar3 = {0, 0, ".?AVUIScrollBar3@@"};
static const struct ReferenceType type_UIScrollBar4 = {0, 0, ".?AVUIScrollBar4@@"};
static const struct ReferenceType type_UIScrollBar_Custom = {0, 0, ".?AVUIScrollBar_Custom@@"};
static const struct ReferenceType type_UIZeroQuestScrollBar = {0, 0, ".?AVUIZeroQuestScrollBar@@"};
static const struct ReferenceType type_UITextButton = {0, 0, ".?AVUITextButton@@"};
static const struct ReferenceType type_UIItemTagButton = {0, 0, ".?AVUIItemTagButton@@"};
static const struct ReferenceType type_UIEditCtrlVarId = {0, 0, ".?AVUIEditCtrlVarId@@"};
static const struct ReferenceType type_UIItemTagOnChat = {0, 0, ".?AVUIItemTagOnChat@@"};
static const struct ReferenceType type_UIItemTagCtrl = {0, 0, ".?AVUIItemTagCtrl@@"};
static const struct ReferenceType type_UIBarGraphPlayer = {0, 0, ".?AVUIBarGraphPlayer@@"};
static const struct ReferenceType type_UIBarGraphHomun = {0, 0, ".?AVUIBarGraphHomun@@"};
static const struct ReferenceType type_UIBarGraph = {0, 0, ".?AVUIBarGraph@@"};
static const struct ReferenceType type_UIBarGraph2 = {0, 0, ".?AVUIBarGraph2@@"};
static const struct ReferenceType type_UIBarGraph3 = {0, 0, ".?AVUIBarGraph3@@"};
static const struct ReferenceType type_UIINT64BarGraph = {0, 0, ".?AVUIINT64BarGraph@@"};
static const struct ReferenceType type_UIPetInfoBarGraph = {0, 0, ".?AVUIPetInfoBarGraph@@"};
static const struct ReferenceType type_UIGage = {0, 0, ".?AVUIGage@@"};
static const struct ReferenceType type_UIRechargeGage = {0, 0, ".?AVUIRechargeGage@@"};
static const struct ReferenceType type_UIPcGage = {0, 0, ".?AVUIPcGage@@"};
static const struct ReferenceType type_UIPlayerGage = {0, 0, ".?AVUIPlayerGage@@"};
static const struct ReferenceType type_UIMonsterGage = {0, 0, ".?AVUIMonsterGage@@"};
static const struct ReferenceType type_UIColorTransBalloonText = {0, 0, ".?AVUIColorTransBalloonText@@"};
static const struct ReferenceType type_UIBitmapButton2 = {0, 0, ".?AVUIBitmapButton2@@"};
static const struct ReferenceType type_UIPackageBoxNameList = {0, 0, ".?AVUIPackageBoxNameList@@"};
static const struct ReferenceType type_UIListBox2 = {0, 0, ".?AVUIListBox2@@"};
static const struct ReferenceType type_UIListBoxID = {0, 0, ".?AVUIListBoxID@@"};
static const struct ReferenceType type_UIListBoxNavi = {0, 0, ".?AVUIListBoxNavi@@"};
static const struct ReferenceType type_UIListBoxEx = {0, 0, ".?AVUIListBoxEx@@"};
static const struct ReferenceType type_UIListBoxTitle = {0, 0, ".?AVUIListBoxTitle@@"};
static const struct ReferenceType type_UIComboBoxEx = {0, 0, ".?AVUIComboBoxEx@@"};
static const struct ReferenceType type_UIComboBoxEx2 = {0, 0, ".?AVUIComboBoxEx2@@"};
static const struct ReferenceType type_UIItemStoreComboBox = {0, 0, ".?AVUIItemStoreComboBox@@"};
static const struct ReferenceType type_UIComboBox = {0, 0, ".?AVUIComboBox@@"};
static const struct ReferenceType type_UIComboBoxID = {0, 0, ".?AVUIComboBoxID@@"};
static const struct ReferenceType type_UIComboBoxDefWnd = {0, 0, ".?AVUIComboBoxDefWnd@@"};
static const struct ReferenceType type_UIComboBoxDef2Wnd = {0, 0, ".?AVUIComboBoxDef2Wnd@@"};
static const struct ReferenceType type_UIItemStoreComboBoxWnd = {0, 0, ".?AVUIItemStoreComboBoxWnd@@"};
static const struct ReferenceType type_UIComboBox_BmpWnd = {0, 0, ".?AVUIComboBox_BmpWnd@@"};
static const struct ReferenceType type_UIComboBoxV2Wnd = {0, 0, ".?AVUIComboBoxV2Wnd@@"};
static const struct ReferenceType type_TableBox = {0, 0, ".?AVTableBox@@"};
static const struct ReferenceType type_UITableBox = {0, 0, ".?AVUITableBox@@"};
static const struct ReferenceType type_UIChatHisBox = {0, 0, ".?AVUIChatHisBox@@"};
static const struct ReferenceType type_UIChatHisBox2 = {0, 0, ".?AVUIChatHisBox2@@"};
static const struct ReferenceType type_UIChatHisBox3 = {0, 0, ".?AVUIChatHisBox3@@"};
static const struct ReferenceType type_UIMemberListBox = {0, 0, ".?AVUIMemberListBox@@"};
static const struct ReferenceType type_UITextViewer = {0, 0, ".?AVUITextViewer@@"};
static const struct ReferenceType type_UIString = {0, 0, ".?AVUIString@@"};
static const struct ReferenceType type_UITextViewEX = {0, 0, ".?AVUITextViewEX@@"};
static const struct ReferenceType type_UIServerListBox = {0, 0, ".?AVUIServerListBox@@"};
static const struct ReferenceType type_UIImgMenu = {0, 0, ".?AVUIImgMenu@@"};
static const struct ReferenceType type_UITVTextButton = {0, 0, ".?AVUITVTextButton@@"};
static const struct ReferenceType type_UITVBitmapButton = {0, 0, ".?AVUITVBitmapButton@@"};
static const struct ReferenceType type_UITreeView = {0, 0, ".?AVUITreeView@@"};
static const struct ReferenceType type_UITabControl = {0, 0, ".?AVUITabControl@@"};
static const struct ReferenceType type_UIShowAndEditTimeCtrl = {0, 0, ".?AVUIShowAndEditTimeCtrl@@"};
static const struct ReferenceType type_UIBasicItemCtrl = {0, 0, ".?AVUIBasicItemCtrl@@"};
static const struct ReferenceType type_UIBasicSkillCtrl = {0, 0, ".?AVUIBasicSkillCtrl@@"};
static const struct ReferenceType type_UINewComboBoxCtrl = {0, 0, ".?AVUINewComboBoxCtrl@@"};
static const struct ReferenceType type_UIItemMoveInfo = {0, 0, ".?AVUIItemMoveInfo@@"};
static const struct ReferenceType type_UIItemTagOnMegaPhone = {0, 0, ".?AVUIItemTagOnMegaPhone@@"};
static const struct ReferenceType type_UIItemTagOnMail = {0, 0, ".?AVUIItemTagOnMail@@"};
static const struct ReferenceType type_UIBmpRadioBtn = {0, 0, ".?AVUIBmpRadioBtn@@"};
static const struct ReferenceType type_UIEditCtrl_Custom = {0, 0, ".?AVUIEditCtrl_Custom@@"};
static const struct ReferenceType type_UIEditCtrl_Currency = {0, 0, ".?AVUIEditCtrl_Currency@@"};
static const struct ReferenceType type_UICaptchaImage = {0, 0, ".?AVUICaptchaImage@@"};
static const struct ReferenceType type_UIStaticItem = {0, 0, ".?AVUIStaticItem@@"};
static const struct ReferenceType type_UIStaticItem_Illust = {0, 0, ".?AVUIStaticItem_Illust@@"};
static const struct ReferenceType type_UINaviLinkButton = {0, 0, ".?AVUINaviLinkButton@@"};
static const struct ReferenceType type_UISlotBmpButton = {0, 0, ".?AVUISlotBmpButton@@"};
static const struct ReferenceType type_UIDescriptRace = {0, 0, ".?AVUIDescriptRace@@"};
static const struct ReferenceType type_UIRPImageWnd = {0, 0, ".?AVUIRPImageWnd@@"};
static const struct ReferenceType type_UIScrollBar_Navi = {0, 0, ".?AVUIScrollBar_Navi@@"};
static const struct ReferenceType type_UIWaitWnd = {0, 0, ".?AVUIWaitWnd@@"};
static const struct ReferenceType type_UIExchangeWnd = {0, 0, ".?AVUIExchangeWnd@@"};
static const struct ReferenceType type_UILoadingWnd = {0, 0, ".?AVUILoadingWnd@@"};
static const struct ReferenceType type_UIToolTipWnd = {0, 0, ".?AVUIToolTipWnd@@"};
static const struct ReferenceType type_UIEquipWnd = {0, 0, ".?AVUIEquipWnd@@"};
static const struct ReferenceType type_UInCashWnd = {0, 0, ".?AVUInCashWnd@@"};
static const struct ReferenceType type_UIItemBaseWnd = {0, 0, ".?AVUIItemBaseWnd@@"};
static const struct ReferenceType type_UIItemParamChangeDisplayWnd = {0, 0, ".?AVUIItemParamChangeDisplayWnd@@"};
static const struct ReferenceType type_UIPasswordWnd = {0, 0, ".?AVUIPasswordWnd@@"};
static const struct ReferenceType type_UIItemDropCntWnd = {0, 0, ".?AVUIItemDropCntWnd@@"};
static const struct ReferenceType type_UIEmailAddressWnd = {0, 0, ".?AVUIEmailAddressWnd@@"};
static const struct ReferenceType type_UISkillNameChangeWnd = {0, 0, ".?AVUISkillNameChangeWnd@@"};
static const struct ReferenceType type_UINotifyItemObtainWnd = {0, 0, ".?AVUINotifyItemObtainWnd@@"};
static const struct ReferenceType type_UINpcEditDialogWnd = {0, 0, ".?AVUINpcEditDialogWnd@@"};
static const struct ReferenceType type_UINpcTextEditDialogWnd = {0, 0, ".?AVUINpcTextEditDialogWnd@@"};
static const struct ReferenceType type_UIStatusWnd = {0, 0, ".?AVUIStatusWnd@@"};
static const struct ReferenceType type_UIItemCollectionWnd = {0, 0, ".?AVUIItemCollectionWnd@@"};
static const struct ReferenceType type_UICombinedCardItemCollectionWnd = {0, 0, ".?AVUICombinedCardItemCollectionWnd@@"};
static const struct ReferenceType type_UIItemCollection_ComparisonWnd = {0, 0, ".?AVUIItemCollection_ComparisonWnd@@"};
static const struct ReferenceType type_UICombinedCardItemCollection_ComparisonWnd = {0, 0, ".?AVUICombinedCardItemCollection_ComparisonWnd@@"};
static const struct ReferenceType type_UISkillDescribeWnd = {0, 0, ".?AVUISkillDescribeWnd@@"};
static const struct ReferenceType type_UICardItemIllustWnd = {0, 0, ".?AVUICardItemIllustWnd@@"};
static const struct ReferenceType type_UIMonsterDisplayWnd = {0, 0, ".?AVUIMonsterDisplayWnd@@"};
static const struct ReferenceType type_UISayDialogWnd = {0, 0, ".?AVUISayDialogWnd@@"};
static const struct ReferenceType type_UIChooseWnd = {0, 0, ".?AVUIChooseWnd@@"};
static const struct ReferenceType type_UIChooseWarpWnd = {0, 0, ".?AVUIChooseWarpWnd@@"};
static const struct ReferenceType type_UIRestartWnd = {0, 0, ".?AVUIRestartWnd@@"};
static const struct ReferenceType type_UIQuitWnd = {0, 0, ".?AVUIQuitWnd@@"};
static const struct ReferenceType type_UINoticeConfirmWnd = {0, 0, ".?AVUINoticeConfirmWnd@@"};
static const struct ReferenceType type_UIChooseSellBuyWnd = {0, 0, ".?AVUIChooseSellBuyWnd@@"};
static const struct ReferenceType type_UIDisconnectedServerMsgWnd = {0, 0, ".?AVUIDisconnectedServerMsgWnd@@"};
static const struct ReferenceType type_UIExchangeAcceptWnd = {0, 0, ".?AVUIExchangeAcceptWnd@@"};
static const struct ReferenceType type_UIJoinPartyAcceptWnd = {0, 0, ".?AVUIJoinPartyAcceptWnd@@"};
static const struct ReferenceType type_UICoupleAcceptWnd = {0, 0, ".?AVUICoupleAcceptWnd@@"};
static const struct ReferenceType type_UIStarPlaceAcceptWnd = {0, 0, ".?AVUIStarPlaceAcceptWnd@@"};
static const struct ReferenceType type_UIMixAcceptWnd = {0, 0, ".?AVUIMixAcceptWnd@@"};
static const struct ReferenceType type_UIYourItemWnd = {0, 0, ".?AVUIYourItemWnd@@"};
static const struct ReferenceType type_UIBabyAcceptWnd = {0, 0, ".?AVUIBabyAcceptWnd@@"};
static const struct ReferenceType type_UIBabyAcceptWnd2 = {0, 0, ".?AVUIBabyAcceptWnd2@@"};
static const struct ReferenceType type_UIJoinGuildAcceptWnd = {0, 0, ".?AVUIJoinGuildAcceptWnd@@"};
static const struct ReferenceType type_UITipOfTheDayWnd = {0, 0, ".?AVUITipOfTheDayWnd@@"};
static const struct ReferenceType type_UIChatRoomMakeWnd = {0, 0, ".?AVUIChatRoomMakeWnd@@"};
static const struct ReferenceType type_UIChatRoomChangeWnd = {0, 0, ".?AVUIChatRoomChangeWnd@@"};
static const struct ReferenceType type_UIPartySettingWnd = {0, 0, ".?AVUIPartySettingWnd@@"};
static const struct ReferenceType type_UIPartyInvitationToWnd = {0, 0, ".?AVUIPartyInvitationToWnd@@"};
static const struct ReferenceType type_UIWhisperWnd = {0, 0, ".?AVUIWhisperWnd@@"};
static const struct ReferenceType type_UIItemIdentifyWnd = {0, 0, ".?AVUIItemIdentifyWnd@@"};
static const struct ReferenceType type_UIPetEggListWnd = {0, 0, ".?AVUIPetEggListWnd@@"};
static const struct ReferenceType type_UIMakingArrowListWnd = {0, 0, ".?AVUIMakingArrowListWnd@@"};
static const struct ReferenceType type_UIItemCompositionWnd = {0, 0, ".?AVUIItemCompositionWnd@@"};
static const struct ReferenceType type_UIMenuWnd = {0, 0, ".?AVUIMenuWnd@@"};
static const struct ReferenceType type_UIComboBoxWnd = {0, 0, ".?AVUIComboBoxWnd@@"};
static const struct ReferenceType type_UIComboBoxIDWnd = {0, 0, ".?AVUIComboBoxIDWnd@@"};
static const struct ReferenceType type_UIPetTamingDeceiveWnd = {0, 0, ".?AVUIPetTamingDeceiveWnd@@"};
static const struct ReferenceType type_UISelectCartWnd = {0, 0, ".?AVUISelectCartWnd@@"};
static const struct ReferenceType type_UISpellListWnd = {0, 0, ".?AVUISpellListWnd@@"};
static const struct ReferenceType type_UIGraffiStrboxWnd = {0, 0, ".?AVUIGraffiStrboxWnd@@"};
static const struct ReferenceType type_UIProhibitListWnd = {0, 0, ".?AVUIProhibitListWnd@@"};
static const struct ReferenceType type_UIBookWnd = {0, 0, ".?AVUIBookWnd@@"};
static const struct ReferenceType type_UIJoinFriendAcceptWnd = {0, 0, ".?AVUIJoinFriendAcceptWnd@@"};
static const struct ReferenceType type_UICommandListWnd = {0, 0, ".?AVUICommandListWnd@@"};
static const struct ReferenceType type_UIStorePasswordInputWnd = {0, 0, ".?AVUIStorePasswordInputWnd@@"};
static const struct ReferenceType type_UISecondPasswordInputWnd = {0, 0, ".?AVUISecondPasswordInputWnd@@"};
static const struct ReferenceType type_UIAutoMessageWnd = {0, 0, ".?AVUIAutoMessageWnd@@"};
static const struct ReferenceType type_UITipboxWnd = {0, 0, ".?AVUITipboxWnd@@"};
static const struct ReferenceType type_CSprRes = {0, 0, ".?AVCSprRes@@"};
static const struct ReferenceType type_CActRes = {0, 0, ".?AVCActRes@@"};
static const struct ReferenceType type_UIShortCutWnd = {0, 0, ".?AVUIShortCutWnd@@"};
static const struct ReferenceType type_UICharInfoBalloonText = {0, 0, ".?AVUICharInfoBalloonText@@"};
static const struct ReferenceType type_UISelCharForUServerWnd = {0, 0, ".?AVUISelCharForUServerWnd@@"};
static const struct ReferenceType type_UIPartyBookingHelpWnd = {0, 0, ".?AVUIPartyBookingHelpWnd@@"};
static const struct ReferenceType type_UISeekPartyWnd = {0, 0, ".?AVUISeekPartyWnd@@"};
static const struct ReferenceType type_UIJobListWnd = {0, 0, ".?AVUIJobListWnd@@"};
static const struct ReferenceType type_UISeekPartyMBWnd = {0, 0, ".?AVUISeekPartyMBWnd@@"};
static const struct ReferenceType type_UISeekPartyListWnd = {0, 0, ".?AVUISeekPartyListWnd@@"};
static const struct ReferenceType type_UIChangeNameWnd = {0, 0, ".?AVUIChangeNameWnd@@"};
static const struct ReferenceType type_UIRebirthWnd = {0, 0, ".?AVUIRebirthWnd@@"};
static const struct ReferenceType type_UISubChatMiniWnd = {0, 0, ".?AVUISubChatMiniWnd@@"};
static const struct ReferenceType type_UIBattleMsgOptionWnd = {0, 0, ".?AVUIBattleMsgOptionWnd@@"};
static const struct ReferenceType type_UISubChatHisWnd = {0, 0, ".?AVUISubChatHisWnd@@"};
static const struct ReferenceType type_UINewChatWnd = {0, 0, ".?AVUINewChatWnd@@"};
static const struct ReferenceType type_UIMemorialDunWnd = {0, 0, ".?AVUIMemorialDunWnd@@"};
static const struct ReferenceType type_UIRoMapWnd = {0, 0, ".?AVUIRoMapWnd@@"};
static const struct ReferenceType type_UIMiniBtnWnd = {0, 0, ".?AVUIMiniBtnWnd@@"};
static const struct ReferenceType type_UIHotKeyWnd = {0, 0, ".?AVUIHotKeyWnd@@"};
static const struct ReferenceType type_UIEscOptionWnd = {0, 0, ".?AVUIEscOptionWnd@@"};
static const struct ReferenceType type_UIQuikSlotWnd = {0, 0, ".?AVUIQuikSlotWnd@@"};
static const struct ReferenceType type_UISerachStoreInfoWnd = {0, 0, ".?AVUISerachStoreInfoWnd@@"};
static const struct ReferenceType type_UIButtonsOfBasicWnd = {0, 0, ".?AVUIButtonsOfBasicWnd@@"};
static const struct ReferenceType type_UINavigationWnd = {0, 0, ".?AVUINavigationWnd@@"};
static const struct ReferenceType type_UINavigationHelpWnd = {0, 0, ".?AVUINavigationHelpWnd@@"};
static const struct ReferenceType type_CUICollectionSystemWnd = {0, 0, ".?AVCUICollectionSystemWnd@@"};
static const struct ReferenceType type_UIGuildInfoManageWnd = {0, 0, ".?AVUIGuildInfoManageWnd@@"};
static const struct ReferenceType type_UIGuildWnd = {0, 0, ".?AVUIGuildWnd@@"};
static const struct ReferenceType type_UIGuildMemberManageWnd = {0, 0, ".?AVUIGuildMemberManageWnd@@"};
static const struct ReferenceType type_UIGuildPositionManageWnd = {0, 0, ".?AVUIGuildPositionManageWnd@@"};
static const struct ReferenceType type_UIGuildSkillWnd = {0, 0, ".?AVUIGuildSkillWnd@@"};
static const struct ReferenceType type_UIGuildBanishedMemberWnd = {0, 0, ".?AVUIGuildBanishedMemberWnd@@"};
static const struct ReferenceType type_UIGuildNoticeWnd = {0, 0, ".?AVUIGuildNoticeWnd@@"};
static const struct ReferenceType type_UIGuildTotalInfoWnd = {0, 0, ".?AVUIGuildTotalInfoWnd@@"};
static const struct ReferenceType type_UIGuildLeaveReasonDescWnd = {0, 0, ".?AVUIGuildLeaveReasonDescWnd@@"};
static const struct ReferenceType type_UIGuildBanReasonDescWnd = {0, 0, ".?AVUIGuildBanReasonDescWnd@@"};
static const struct ReferenceType type_UIGuildHelperWnd = {0, 0, ".?AVUIGuildHelperWnd@@"};
static const struct ReferenceType type_UIGuildTipWnd = {0, 0, ".?AVUIGuildTipWnd@@"};
static const struct ReferenceType type_UICreateGuildWnd = {0, 0, ".?AVUICreateGuildWnd@@"};
static const struct ReferenceType type_UIAgitMessageBox = {0, 0, ".?AVUIAgitMessageBox@@"};
static const struct ReferenceType type_UIListBoxAgitMenu = {0, 0, ".?AVUIListBoxAgitMenu@@"};
static const struct ReferenceType type_UIListBoxAgit = {0, 0, ".?AVUIListBoxAgit@@"};
static const struct ReferenceType type_UIParaItemPurchaseWnd = {0, 0, ".?AVUIParaItemPurchaseWnd@@"};
static const struct ReferenceType type_UIParaItemShopWnd = {0, 0, ".?AVUIParaItemShopWnd@@"};
static const struct ReferenceType type_UIEditCtrlWheel = {0, 0, ".?AVUIEditCtrlWheel@@"};
static const struct ReferenceType type_UIItemStoreSubWnd = {0, 0, ".?AVUIItemStoreSubWnd@@"};
static const struct ReferenceType type_UIItemStoreFindWnd = {0, 0, ".?AVUIItemStoreFindWnd@@"};
static const struct ReferenceType type_UIItemStoreWnd = {0, 0, ".?AVUIItemStoreWnd@@"};
static const struct ReferenceType type_UISkillListWnd = {0, 0, ".?AVUISkillListWnd@@"};
static const struct ReferenceType type_UIItemShopWnd = {0, 0, ".?AVUIItemShopWnd@@"};
static const struct ReferenceType type_UIItemShopWnd2 = {0, 0, ".?AVUIItemShopWnd2@@"};
static const struct ReferenceType type_UIItemPurchaseWnd = {0, 0, ".?AVUIItemPurchaseWnd@@"};
static const struct ReferenceType type_UIItemSellWnd = {0, 0, ".?AVUIItemSellWnd@@"};
static const struct ReferenceType type_UIItemSellWnd2 = {0, 0, ".?AVUIItemSellWnd2@@"};
static const struct ReferenceType type_UIMerchantItemShopWnd = {0, 0, ".?AVUIMerchantItemShopWnd@@"};
static const struct ReferenceType type_UIMerchantItemMyShopWnd = {0, 0, ".?AVUIMerchantItemMyShopWnd@@"};
static const struct ReferenceType type_UIMerchantItemPurchaseMSGWnd = {0, 0, ".?AVUIMerchantItemPurchaseMSGWnd@@"};
static const struct ReferenceType type_UIMerchantItemPurchaseWnd = {0, 0, ".?AVUIMerchantItemPurchaseWnd@@"};
static const struct ReferenceType type_UINoticeMessageBox_LockSellItem = {0, 0, ".?AVUINoticeMessageBox_LockSellItem@@"};
static const struct ReferenceType type_UIItemWnd = {0, 0, ".?AVUIItemWnd@@"};
static const struct ReferenceType type_UIMerchantItemWnd = {0, 0, ".?AVUIMerchantItemWnd@@"};
static const struct ReferenceType type_UIMerchantMirrorItemWnd = {0, 0, ".?AVUIMerchantMirrorItemWnd@@"};
static const struct ReferenceType type_UIMerchantShopMakeWnd = {0, 0, ".?AVUIMerchantShopMakeWnd@@"};
static const struct ReferenceType type_UIMinimapZoomWnd = {0, 0, ".?AVUIMinimapZoomWnd@@"};
static const struct ReferenceType type_UINotifyLevelUpWnd = {0, 0, ".?AVUINotifyLevelUpWnd@@"};
static const struct ReferenceType type_UINotifyJobLevelUpWnd = {0, 0, ".?AVUINotifyJobLevelUpWnd@@"};
static const struct ReferenceType type_UINotifyQuestWnd = {0, 0, ".?AVUINotifyQuestWnd@@"};
static const struct ReferenceType type_UInCash_CallWnd = {0, 0, ".?AVUInCash_CallWnd@@"};
static const struct ReferenceType type_UISecondPasswordEditWnd = {0, 0, ".?AVUISecondPasswordEditWnd@@"};
static const struct ReferenceType type_UIEmotionWnd = {0, 0, ".?AVUIEmotionWnd@@"};
static const struct ReferenceType type_CEffectPrim = {0, 0, ".?AVCEffectPrim@@"};
static const struct ReferenceType type_CEffect = {0, 0, ".?AVCEffect@@"};
static const struct ReferenceType type_CTetraBallEffect = {0, 0, ".?AVCTetraBallEffect@@"};
static const struct ReferenceType type_CMousePointer = {0, 0, ".?AVCMousePointer@@"};
static const struct ReferenceType type_C3dGrannyGameActor = {0, 0, ".?AVC3dGrannyGameActor@@"};
static const struct ReferenceType type_CPc = {0, 0, ".?AVCPc@@"};
static const struct ReferenceType type_UIVerticalNameBalloonText = {0, 0, ".?AVUIVerticalNameBalloonText@@"};
static const struct ReferenceType type_CDummyEffectLauncher = {0, 0, ".?AVCDummyEffectLauncher@@"};
static const struct ReferenceType type_UIParaResultWnd = {0, 0, ".?AVUIParaResultWnd@@"};
static const struct ReferenceType type_UIGraphicSettingWnd = {0, 0, ".?AVUIGraphicSettingWnd@@"};
static const struct ReferenceType type_CItem = {0, 0, ".?AVCItem@@"};
static const struct ReferenceType type_CCorpse = {0, 0, ".?AVCCorpse@@"};
static const struct ReferenceType type_CGndRiser = {0, 0, ".?AVCGndRiser@@"};
static const struct ReferenceType type_CSoundMaker = {0, 0, ".?AVCSoundMaker@@"};
static const struct ReferenceType type_CBlowEffect = {0, 0, ".?AVCBlowEffect@@"};
static const struct ReferenceType type_CBlurPC = {0, 0, ".?AVCBlurPC@@"};
static const struct ReferenceType type_CHalluPC = {0, 0, ".?AVCHalluPC@@"};
static const struct ReferenceType type_CHalluPC2 = {0, 0, ".?AVCHalluPC2@@"};
static const struct ReferenceType type_CHalluNPC = {0, 0, ".?AVCHalluNPC@@"};
static const struct ReferenceType type_CMasterChaser = {0, 0, ".?AVCMasterChaser@@"};
static const struct ReferenceType type_CNumEffect = {0, 0, ".?AVCNumEffect@@"};
static const struct ReferenceType type_CHalluPlant = {0, 0, ".?AVCHalluPlant@@"};
static const struct ReferenceType type_CGrannyPc = {0, 0, ".?AVCGrannyPc@@"};
static const struct ReferenceType type_CPlayer = {0, 0, ".?AVCPlayer@@"};
static const struct ReferenceType type_UIEntryQueueWnd = {0, 0, ".?AVUIEntryQueueWnd@@"};
static const struct ReferenceType type_UIEntryQueueStandByWnd = {0, 0, ".?AVUIEntryQueueStandByWnd@@"};
static const struct ReferenceType type_CMsgEffect = {0, 0, ".?AVCMsgEffect@@"};
static const struct ReferenceType type_CGroundScope = {0, 0, ".?AVCGroundScope@@"};
static const struct ReferenceType type_CEffectMaker = {0, 0, ".?AVCEffectMaker@@"};
static const struct ReferenceType type_CSkill = {0, 0, ".?AVCSkill@@"};
static const struct ReferenceType type_CGraffiSkill = {0, 0, ".?AVCGraffiSkill@@"};
static const struct ReferenceType type_CArrowEffect = {0, 0, ".?AVCArrowEffect@@"};
static const struct ReferenceType type_CMandraAttackEffect = {0, 0, ".?AVCMandraAttackEffect@@"};
static const struct ReferenceType type_UIEnchant_Tab = {0, 0, ".?AVUIEnchant_Tab@@"};
static const struct ReferenceType type_UIEnchant_Group = {0, 0, ".?AVUIEnchant_Group@@"};
static const struct ReferenceType type_UIEnchant_ScrollBar = {0, 0, ".?AVUIEnchant_ScrollBar@@"};
static const struct ReferenceType type_UIEnchant_List = {0, 0, ".?AVUIEnchant_List@@"};
static const struct ReferenceType type_UIEnchant_TargetList = {0, 0, ".?AVUIEnchant_TargetList@@"};
static const struct ReferenceType type_UIEnchant_Item = {0, 0, ".?AVUIEnchant_Item@@"};
static const struct ReferenceType type_UIEnchant_Button = {0, 0, ".?AVUIEnchant_Button@@"};
static const struct ReferenceType type_UIEnchant_Info = {0, 0, ".?AVUIEnchant_Info@@"};
static const struct ReferenceType type_UIEnchantWnd = {0, 0, ".?AVUIEnchantWnd@@"};
static const struct ReferenceType type_UIItemReformWnd_List = {0, 0, ".?AVUIItemReformWnd_List@@"};
static const struct ReferenceType type_UIItemReformWnd_Item = {0, 0, ".?AVUIItemReformWnd_Item@@"};
static const struct ReferenceType type_UIItemReformWnd_DetailBtn = {0, 0, ".?AVUIItemReformWnd_DetailBtn@@"};
static const struct ReferenceType type_UIItemReformWnd_DetailMsg = {0, 0, ".?AVUIItemReformWnd_DetailMsg@@"};
static const struct ReferenceType type_UIItemReformWnd = {0, 0, ".?AVUIItemReformWnd@@"};
static const struct ReferenceType type_UILapineScrollBar = {0, 0, ".?AVUILapineScrollBar@@"};
static const struct ReferenceType type_UIIngredientInfoWnd = {0, 0, ".?AVUIIngredientInfoWnd@@"};
static const struct ReferenceType type_UILapineBox_IngredientListWnd = {0, 0, ".?AVUILapineBox_IngredientListWnd@@"};
static const struct ReferenceType type_UIScrollWnd = {0, 0, ".?AVUIScrollWnd@@"};
static const struct ReferenceType type_UILapineBoxWnd = {0, 0, ".?AVUILapineBoxWnd@@"};
static const struct ReferenceType type_UILapineUpgradeBox_IngredientInfoWnd = {0, 0, ".?AVUILapineUpgradeBox_IngredientInfoWnd@@"};
static const struct ReferenceType type_UILapineUpgradeBox_IngredientListWnd = {0, 0, ".?AVUILapineUpgradeBox_IngredientListWnd@@"};
static const struct ReferenceType type_UILapineUpgradeBoxWnd = {0, 0, ".?AVUILapineUpgradeBoxWnd@@"};
static const struct ReferenceType type_UIReputeWnd = {0, 0, ".?AVUIReputeWnd@@"};
static const struct ReferenceType type_CAcidifiedThrow = {0, 0, ".?AVCAcidifiedThrow@@"};
static const struct ReferenceType type_CAxeBoomerang = {0, 0, ".?AVCAxeBoomerang@@"};
static const struct ReferenceType type_CAnimatedEmitterEffect = {0, 0, ".?AVCAnimatedEmitterEffect@@"};
static const struct ReferenceType type_CEmitterEffect = {0, 0, ".?AVCEmitterEffect@@"};
static const struct ReferenceType type_CCartCannon = {0, 0, ".?AVCCartCannon@@"};
static const struct ReferenceType type_CBindTrapEffect = {0, 0, ".?AVCBindTrapEffect@@"};
static const struct ReferenceType type_CCloud = {0, 0, ".?AVCCloud@@"};
static const struct ReferenceType type_CCloudKillEffect = {0, 0, ".?AVCCloudKillEffect@@"};
static const struct ReferenceType type_CCommonFloorEffect = {0, 0, ".?AVCCommonFloorEffect@@"};
static const struct ReferenceType type_CEvilLandEffect = {0, 0, ".?AVCEvilLandEffect@@"};
static const struct ReferenceType type_CEvilsPawEffect = {0, 0, ".?AVCEvilsPawEffect@@"};
static const struct ReferenceType type_CFortuneKiss = {0, 0, ".?AVCFortuneKiss@@"};
static const struct ReferenceType type_CFriggSongEffect = {0, 0, ".?AVCFriggSongEffect@@"};
static const struct ReferenceType type_CChangeSizeEffect = {0, 0, ".?AVCChangeSizeEffect@@"};
static const struct ReferenceType type_CChargingPierceEffect = {0, 0, ".?AVCChargingPierceEffect@@"};
static const struct ReferenceType type_CHitLineEffect = {0, 0, ".?AVCHitLineEffect@@"};
static const struct ReferenceType type_CMeteorEffect = {0, 0, ".?AVCMeteorEffect@@"};
static const struct ReferenceType type_CNyanggrassEffect = {0, 0, ".?AVCNyanggrassEffect@@"};
static const struct ReferenceType type_CPowderingEffect = {0, 0, ".?AVCPowderingEffect@@"};
static const struct ReferenceType type_CRectUpEffect = {0, 0, ".?AVCRectUpEffect@@"};
static const struct ReferenceType type_CRichManKim = {0, 0, ".?AVCRichManKim@@"};
static const struct ReferenceType type_CRootTwistEffect = {0, 0, ".?AVCRootTwistEffect@@"};
static const struct ReferenceType type_CSpriteMableEffect = {0, 0, ".?AVCSpriteMableEffect@@"};
static const struct ReferenceType type_CStemSpearEffect = {0, 0, ".?AVCStemSpearEffect@@"};
static const struct ReferenceType type_CSvgSpiritEffect = {0, 0, ".?AVCSvgSpiritEffect@@"};
static const struct ReferenceType type_CTunaPartyEffect = {0, 0, ".?AVCTunaPartyEffect@@"};
static const struct ReferenceType type_CWarningPlaneEffect = {0, 0, ".?AVCWarningPlaneEffect@@"};
static const struct ReferenceType type_CFallenAngelEffect = {0, 0, ".?AVCFallenAngelEffect@@"};
static const struct ReferenceType type_CGravityControlEffect = {0, 0, ".?AVCGravityControlEffect@@"};
static const struct ReferenceType type_CJupitelThunderHit = {0, 0, ".?AVCJupitelThunderHit@@"};
static const struct ReferenceType type_CNewCloudKillEffect = {0, 0, ".?AVCNewCloudKillEffect@@"};
static const struct ReferenceType type_CRandomOptEffect = {0, 0, ".?AVCRandomOptEffect@@"};
static const struct ReferenceType type_CRichsCoinEffect = {0, 0, ".?AVCRichsCoinEffect@@"};
static const struct ReferenceType type_CServantWeaponEffect = {0, 0, ".?AVCServantWeaponEffect@@"};
static const struct ReferenceType type_CSevereRainStorm = {0, 0, ".?AVCSevereRainStorm@@"};
static const struct ReferenceType type_CShowEffect = {0, 0, ".?AVCShowEffect@@"};
static const struct ReferenceType type_CSoulCollectEffect = {0, 0, ".?AVCSoulCollectEffect@@"};
static const struct ReferenceType type_CSoulUnityEffect = {0, 0, ".?AVCSoulUnityEffect@@"};
static const struct ReferenceType type_CStarEmperorEffect = {0, 0, ".?AVCStarEmperorEffect@@"};
static const struct ReferenceType type_CSunStanceEffect = {0, 0, ".?AVCSunStanceEffect@@"};
static const struct ReferenceType type_CTriangleShotArrow = {0, 0, ".?AVCTriangleShotArrow@@"};
static const struct ReferenceType type_CBash = {0, 0, ".?AVCBash@@"};
static const struct ReferenceType type_CCellRangeEffect = {0, 0, ".?AVCCellRangeEffect@@"};
static const struct ReferenceType type_CCherryBlossomEffect = {0, 0, ".?AVCCherryBlossomEffect@@"};
static const struct ReferenceType type_CGlassWall = {0, 0, ".?AVCGlassWall@@"};
static const struct ReferenceType type_CHealSp = {0, 0, ".?AVCHealSp@@"};
static const struct ReferenceType type_CLevel150Effect = {0, 0, ".?AVCLevel150Effect@@"};
static const struct ReferenceType type_CLevel150SubEffect = {0, 0, ".?AVCLevel150SubEffect@@"};
static const struct ReferenceType type_CMagicFloorEffect = {0, 0, ".?AVCMagicFloorEffect@@"};
static const struct ReferenceType type_CMagnumBreak = {0, 0, ".?AVCMagnumBreak@@"};
static const struct ReferenceType type_CMapChainEffect = {0, 0, ".?AVCMapChainEffect@@"};
static const struct ReferenceType type_CMappillarEffect = {0, 0, ".?AVCMappillarEffect@@"};
static const struct ReferenceType type_CSquareRangeEffect = {0, 0, ".?AVCSquareRangeEffect@@"};
static const struct ReferenceType type_CSteal = {0, 0, ".?AVCSteal@@"};
static const struct ReferenceType type_CElectricEffect = {0, 0, ".?AVCElectricEffect@@"};
static const struct ReferenceType type_CEnergyOrbEffect = {0, 0, ".?AVCEnergyOrbEffect@@"};
static const struct ReferenceType type_CEZ2STREffect = {0, 0, ".?AVCEZ2STREffect@@"};
static const struct ReferenceType type_CFlowerCastingEffect = {0, 0, ".?AVCFlowerCastingEffect@@"};
static const struct ReferenceType type_CHealEffect = {0, 0, ".?AVCHealEffect@@"};
static const struct ReferenceType type_CLevel99Effect = {0, 0, ".?AVCLevel99Effect@@"};
static const struct ReferenceType type_CLevel99Orb1Effect = {0, 0, ".?AVCLevel99Orb1Effect@@"};
static const struct ReferenceType type_CLevel99Orb2Effect = {0, 0, ".?AVCLevel99Orb2Effect@@"};
static const struct ReferenceType type_CLightSphereEffect = {0, 0, ".?AVCLightSphereEffect@@"};
static const struct ReferenceType type_CMapMagicZoneEffect = {0, 0, ".?AVCMapMagicZoneEffect@@"};
static const struct ReferenceType type_CPokJukEffect = {0, 0, ".?AVCPokJukEffect@@"};
static const struct ReferenceType type_CRainbowEffect = {0, 0, ".?AVCRainbowEffect@@"};
static const struct ReferenceType type_CRedLineEffect = {0, 0, ".?AVCRedLineEffect@@"};
static const struct ReferenceType type_CRotateBgEffect = {0, 0, ".?AVCRotateBgEffect@@"};
static const struct ReferenceType type_CRotateItemEffect = {0, 0, ".?AVCRotateItemEffect@@"};
static const struct ReferenceType type_CRotateLineEffect = {0, 0, ".?AVCRotateLineEffect@@"};
static const struct ReferenceType type_CSaintCastingEffect = {0, 0, ".?AVCSaintCastingEffect@@"};
static const struct ReferenceType type_CScatterEffect = {0, 0, ".?AVCScatterEffect@@"};
static const struct ReferenceType type_CSmokeEffect = {0, 0, ".?AVCSmokeEffect@@"};
static const struct ReferenceType type_CSphereWind2Effect = {0, 0, ".?AVCSphereWind2Effect@@"};
static const struct ReferenceType type_CSphereWindEffect = {0, 0, ".?AVCSphereWindEffect@@"};
static const struct ReferenceType type_CSpriteEffect = {0, 0, ".?AVCSpriteEffect@@"};
static const struct ReferenceType type_CStarEffect = {0, 0, ".?AVCStarEffect@@"};
static const struct ReferenceType type_CStormGustEffect = {0, 0, ".?AVCStormGustEffect@@"};
static const struct ReferenceType type_CStormKickEffect = {0, 0, ".?AVCStormKickEffect@@"};
static const struct ReferenceType type_CThrowItemEffect = {0, 0, ".?AVCThrowItemEffect@@"};
static const struct ReferenceType type_CVolcanoEffect = {0, 0, ".?AVCVolcanoEffect@@"};
static const struct ReferenceType type_CAstralStrikeEffect = {0, 0, ".?AVCAstralStrikeEffect@@"};
static const struct ReferenceType type_CAMachineEffect = {0, 0, ".?AVCAMachineEffect@@"};
static const struct ReferenceType type_CBlastForge = {0, 0, ".?AVCBlastForge@@"};
static const struct ReferenceType type_CCaneOfEvilEyeEffect = {0, 0, ".?AVCCaneOfEvilEyeEffect@@"};
static const struct ReferenceType type_CEZ2STREffectEx = {0, 0, ".?AVCEZ2STREffectEx@@"};
static const struct ReferenceType type_CFuumaShouakuEffect = {0, 0, ".?AVCFuumaShouakuEffect@@"};
static const struct ReferenceType type_CJupitelThunderStormEffect = {0, 0, ".?AVCJupitelThunderStormEffect@@"};
static const struct ReferenceType type_CClimaxEffect = {0, 0, ".?AVCClimaxEffect@@"};
static const struct ReferenceType type_CCompetentia = {0, 0, ".?AVCCompetentia@@"};
static const struct ReferenceType type_CCresciveBolt = {0, 0, ".?AVCCresciveBolt@@"};
static const struct ReferenceType type_CCrimsonArrowEffect = {0, 0, ".?AVCCrimsonArrowEffect@@"};
static const struct ReferenceType type_CCurseOfCubeEffect = {0, 0, ".?AVCCurseOfCubeEffect@@"};
static const struct ReferenceType type_CCylinderEffect = {0, 0, ".?AVCCylinderEffect@@"};
static const struct ReferenceType type_CDancingKnifeEffect = {0, 0, ".?AVCDancingKnifeEffect@@"};
static const struct ReferenceType type_CEternalSlashAttackEffect = {0, 0, ".?AVCEternalSlashAttackEffect@@"};
static const struct ReferenceType type_CEternalSlashCountEffect = {0, 0, ".?AVCEternalSlashCountEffect@@"};
static const struct ReferenceType type_CFloralFlareRoad = {0, 0, ".?AVCFloralFlareRoad@@"};
static const struct ReferenceType type_CFromTheAbyssEffect = {0, 0, ".?AVCFromTheAbyssEffect@@"};
static const struct ReferenceType type_CFullScreenEffect = {0, 0, ".?AVCFullScreenEffect@@"};
static const struct ReferenceType type_CGaleStorm = {0, 0, ".?AVCGaleStorm@@"};
static const struct ReferenceType type_CHawkVumerangEffect = {0, 0, ".?AVCHawkVumerangEffect@@"};
static const struct ReferenceType type_CItemDropEffect = {0, 0, ".?AVCItemDropEffect@@"};
static const struct ReferenceType type_CPetitioEffect = {0, 0, ".?AVCPetitioEffect@@"};
static const struct ReferenceType type_CPneumaticusProcellaEffect = {0, 0, ".?AVCPneumaticusProcellaEffect@@"};
static const struct ReferenceType type_CRandomHitEffect = {0, 0, ".?AVCRandomHitEffect@@"};
static const struct ReferenceType type_CSavageImpactEffect = {0, 0, ".?AVCSavageImpactEffect@@"};
static const struct ReferenceType type_CShadowExceed = {0, 0, ".?AVCShadowExceed@@"};
static const struct ReferenceType type_CStormCannonEffect = {0, 0, ".?AVCStormCannonEffect@@"};
static const struct ReferenceType type_CTalismanMarkEffect = {0, 0, ".?AVCTalismanMarkEffect@@"};
static const struct ReferenceType type_CViolentQuakeEffect = {0, 0, ".?AVCViolentQuakeEffect@@"};
static const struct ReferenceType type_CWindEffect = {0, 0, ".?AVCWindEffect@@"};
static const struct ReferenceType type_UIDragMiniPartyWnd = {0, 0, ".?AVUIDragMiniPartyWnd@@"};
static const struct ReferenceType type_UIFriendOptionWnd = {0, 0, ".?AVUIFriendOptionWnd@@"};
static const struct ReferenceType type_UIJobIconButton = {0, 0, ".?AVUIJobIconButton@@"};
static const struct ReferenceType type_UIMessengerGroupWnd = {0, 0, ".?AVUIMessengerGroupWnd@@"};
static const struct ReferenceType type_UIMiniPartyInfoWnd = {0, 0, ".?AVUIMiniPartyInfoWnd@@"};
static const struct ReferenceType type_UIMiniPartyWnd = {0, 0, ".?AVUIMiniPartyWnd@@"};
static const struct ReferenceType type_C3dAttr = {0, 0, ".?AVC3dAttr@@"};
static const struct ReferenceType type_CEZeffectRes = {0, 0, ".?AVCEZeffectRes@@"};
static const struct ReferenceType type_CGndRes = {0, 0, ".?AVCGndRes@@"};
static const struct ReferenceType type_CImfRes = {0, 0, ".?AVCImfRes@@"};
static const struct ReferenceType type_C3dModelRes = {0, 0, ".?AVC3dModelRes@@"};
static const struct ReferenceType type_C3dModelResV1 = {0, 0, ".?AVC3dModelResV1@@"};
static const struct ReferenceType type_C3dModelResV2 = {0, 0, ".?AVC3dModelResV2@@"};
static const struct ReferenceType type_C3dAnimModelRes = {0, 0, ".?AVC3dAnimModelRes@@"};
static const struct ReferenceType type_C3dGrannyModelRes = {0, 0, ".?AVC3dGrannyModelRes@@"};
static const struct ReferenceType type_CWave = {0, 0, ".?AVCWave@@"};
static const struct ReferenceType type_C3dWorldRes = {0, 0, ".?AVC3dWorldRes@@"};
static const struct ReferenceType type_UIAchScrollBar = {0, 0, ".?AVUIAchScrollBar@@"};
static const struct ReferenceType type_UIAchTV = {0, 0, ".?AVUIAchTV@@"};
static const struct ReferenceType type_UIAchTab = {0, 0, ".?AVUIAchTab@@"};
static const struct ReferenceType type_UICustomGageBar = {0, 0, ".?AVUICustomGageBar@@"};
static const struct ReferenceType type_UIAchUpperWnd = {0, 0, ".?AVUIAchUpperWnd@@"};
static const struct ReferenceType type_UIToolTipBitmapWnd = {0, 0, ".?AVUIToolTipBitmapWnd@@"};
static const struct ReferenceType type_UIAchViewWnd = {0, 0, ".?AVUIAchViewWnd@@"};
static const struct ReferenceType type_UIAchListWnd = {0, 0, ".?AVUIAchListWnd@@"};
static const struct ReferenceType type_UIAchSummaryWnd = {0, 0, ".?AVUIAchSummaryWnd@@"};
static const struct ReferenceType type_UIAchievementWnd = {0, 0, ".?AVUIAchievementWnd@@"};
static const struct ReferenceType type_UIAchTracingWnd = {0, 0, ".?AVUIAchTracingWnd@@"};
static const struct ReferenceType type_UIJobSlotButton = {0, 0, ".?AVUIJobSlotButton@@"};
static const struct ReferenceType type_UIRegisterPartyWnd = {0, 0, ".?AVUIRegisterPartyWnd@@"};
static const struct ReferenceType type_UIApplyForPartyWnd = {0, 0, ".?AVUIApplyForPartyWnd@@"};
static const struct ReferenceType type_UIPartyInfoWnd = {0, 0, ".?AVUIPartyInfoWnd@@"};
static const struct ReferenceType type_UIAdvenPartyBoardWnd = {0, 0, ".?AVUIAdvenPartyBoardWnd@@"};
static const struct ReferenceType type_UIRequestJoinPartyWnd = {0, 0, ".?AVUIRequestJoinPartyWnd@@"};
static const struct ReferenceType type_CMergeItemWnd = {0, 0, ".?AVCMergeItemWnd@@"};
static const struct ReferenceType type_UIBartermarketWnd = {0, 0, ".?AVUIBartermarketWnd@@"};
static const struct ReferenceType type_UIBarterItemPurchaseWnd = {0, 0, ".?AVUIBarterItemPurchaseWnd@@"};
static const struct ReferenceType type_UIPurchaseButton = {0, 0, ".?AVUIPurchaseButton@@"};
static const struct ReferenceType type_UIExpandedBartermarketWnd = {0, 0, ".?AVUIExpandedBartermarketWnd@@"};
static const struct ReferenceType type_UIExpandedBarterItemPurchaseWnd = {0, 0, ".?AVUIExpandedBarterItemPurchaseWnd@@"};
static const struct ReferenceType type_UIExpandedBarterPurchaseButton = {0, 0, ".?AVUIExpandedBarterPurchaseButton@@"};
static const struct ReferenceType type_UIMenuIcon = {0, 0, ".?AVUIMenuIcon@@"};
static const struct ReferenceType type_UIMenuIconWnd = {0, 0, ".?AVUIMenuIconWnd@@"};
static const struct ReferenceType type_UIClanInfoManageWnd = {0, 0, ".?AVUIClanInfoManageWnd@@"};
static const struct ReferenceType type_UIClanWnd = {0, 0, ".?AVUIClanWnd@@"};
static const struct ReferenceType type_UIEmotionListWnd = {0, 0, ".?AVUIEmotionListWnd@@"};
static const struct ReferenceType type_UIBarGraph4 = {0, 0, ".?AVUIBarGraph4@@"};
static const struct ReferenceType type_UILoginWnd = {0, 0, ".?AVUILoginWnd@@"};
static const struct ReferenceType type_UISelectServerWnd = {0, 0, ".?AVUISelectServerWnd@@"};
static const struct ReferenceType type_UIMakeCharWnd = {0, 0, ".?AVUIMakeCharWnd@@"};
static const struct ReferenceType type_UIMessageBox = {0, 0, ".?AVUIMessageBox@@"};
static const struct ReferenceType type_UIMessageBoxAutoreturn = {0, 0, ".?AVUIMessageBoxAutoreturn@@"};
static const struct ReferenceType type_UILoginOTPWnd = {0, 0, ".?AVUILoginOTPWnd@@"};
static const struct ReferenceType type_UINoticeWnd = {0, 0, ".?AVUINoticeWnd@@"};
static const struct ReferenceType type_UICntWnd = {0, 0, ".?AVUICntWnd@@"};
static const struct ReferenceType type_UIAllyGuildAcceptWnd = {0, 0, ".?AVUIAllyGuildAcceptWnd@@"};
static const struct ReferenceType type_UIChatRoomWnd = {0, 0, ".?AVUIChatRoomWnd@@"};
static const struct ReferenceType type_UIMonsterInfoWnd = {0, 0, ".?AVUIMonsterInfoWnd@@"};
static const struct ReferenceType type_UITalkboxTrapInputWnd = {0, 0, ".?AVUITalkboxTrapInputWnd@@"};
static const struct ReferenceType type_UIPetInfoWnd = {0, 0, ".?AVUIPetInfoWnd@@"};
static const struct ReferenceType type_UIHomunInfoWnd = {0, 0, ".?AVUIHomunInfoWnd@@"};
static const struct ReferenceType type_UIMerInfoWnd = {0, 0, ".?AVUIMerInfoWnd@@"};
static const struct ReferenceType type_UIReplayButton = {0, 0, ".?AVUIReplayButton@@"};
static const struct ReferenceType type_UIReplayControlWnd = {0, 0, ".?AVUIReplayControlWnd@@"};
static const struct ReferenceType type_UIReplayListBox = {0, 0, ".?AVUIReplayListBox@@"};
static const struct ReferenceType type_UISelectReplayDataWnd = {0, 0, ".?AVUISelectReplayDataWnd@@"};
static const struct ReferenceType type_UIReplayRECControlWnd = {0, 0, ".?AVUIReplayRECControlWnd@@"};
static const struct ReferenceType type_UIBank_NewWnd = {0, 0, ".?AVUIBank_NewWnd@@"};
static const struct ReferenceType type_UINormalMB = {0, 0, ".?AVUINormalMB@@"};
static const struct ReferenceType type_UIQuestDisplay = {0, 0, ".?AVUIQuestDisplay@@"};
static const struct ReferenceType type_UIGuild_Storage_Log = {0, 0, ".?AVUIGuild_Storage_Log@@"};
static const struct ReferenceType type_UIOpenMailBoxWnd = {0, 0, ".?AVUIOpenMailBoxWnd@@"};
static const struct ReferenceType type_UISecondCostumeWnd = {0, 0, ".?AVUISecondCostumeWnd@@"};
static const struct ReferenceType type_UIMileageWnd = {0, 0, ".?AVUIMileageWnd@@"};
static const struct ReferenceType type_UIOpenRoulletteWnd = {0, 0, ".?AVUIOpenRoulletteWnd@@"};
static const struct ReferenceType type_UITransMenuWnd = {0, 0, ".?AVUITransMenuWnd@@"};
static const struct ReferenceType type_UIShowWarpWnd = {0, 0, ".?AVUIShowWarpWnd@@"};
static const struct ReferenceType type_UIStaticText_LF = {0, 0, ".?AVUIStaticText_LF@@"};
static const struct ReferenceType type_UINoteWnd_Scroll = {0, 0, ".?AVUINoteWnd_Scroll@@"};
static const struct ReferenceType type_UINoteWnd = {0, 0, ".?AVUINoteWnd@@"};
static const struct ReferenceType type_UIRPImageWnd2 = {0, 0, ".?AVUIRPImageWnd2@@"};
static const struct ReferenceType type_UIAccountLimitedToolWnd = {0, 0, ".?AVUIAccountLimitedToolWnd@@"};
static const struct ReferenceType type_UIHotkeyGuideWnd = {0, 0, ".?AVUIHotkeyGuideWnd@@"};
static const struct ReferenceType type_UIIllustWnd = {0, 0, ".?AVUIIllustWnd@@"};
static const struct ReferenceType type_UIKeyStrokeWnd = {0, 0, ".?AVUIKeyStrokeWnd@@"};
static const struct ReferenceType type_UICompositionWnd = {0, 0, ".?AVUICompositionWnd@@"};
static const struct ReferenceType type_UICandidateWnd = {0, 0, ".?AVUICandidateWnd@@"};
static const struct ReferenceType type_UIRoundBoxTooltipWnd = {0, 0, ".?AVUIRoundBoxTooltipWnd@@"};
static const struct ReferenceType type_UIColoringTextViewer = {0, 0, ".?AVUIColoringTextViewer@@"};
static const struct ReferenceType type_UIMiniMapWnd = {0, 0, ".?AVUIMiniMapWnd@@"};
static const struct ReferenceType type_UIEditctrlWnd = {0, 0, ".?AVUIEditctrlWnd@@"};
static const struct ReferenceType type_UIMerchantItemLogWnd = {0, 0, ".?AVUIMerchantItemLogWnd@@"};
static const struct ReferenceType type_UIMakeTargetListWnd = {0, 0, ".?AVUIMakeTargetListWnd@@"};
static const struct ReferenceType type_UIItemRepairWnd = {0, 0, ".?AVUIItemRepairWnd@@"};
static const struct ReferenceType type_UIWeaponRefineWnd = {0, 0, ".?AVUIWeaponRefineWnd@@"};
static const struct ReferenceType type_UIMakeTargetProcessWnd = {0, 0, ".?AVUIMakeTargetProcessWnd@@"};
static const struct ReferenceType type_UIMakeTargetResultWnd = {0, 0, ".?AVUIMakeTargetResultWnd@@"};
static const struct ReferenceType type_UIPetEvolutionWnd = {0, 0, ".?AVUIPetEvolutionWnd@@"};
static const struct ReferenceType type_UITagButton = {0, 0, ".?AVUITagButton@@"};
static const struct ReferenceType type_UITagListBox = {0, 0, ".?AVUITagListBox@@"};
static const struct ReferenceType type_UIViewCameraInfoWnd = {0, 0, ".?AVUIViewCameraInfoWnd@@"};
static const struct ReferenceType type_UICashShopWnd = {0, 0, ".?AVUICashShopWnd@@"};
static const struct ReferenceType type_UICashShopWnd_Banner = {0, 0, ".?AVUICashShopWnd_Banner@@"};
static const struct ReferenceType type_UICashShopWnd_CartItem = {0, 0, ".?AVUICashShopWnd_CartItem@@"};
static const struct ReferenceType type_UICashShopWnd_Cart = {0, 0, ".?AVUICashShopWnd_Cart@@"};
static const struct ReferenceType type_UICashShopWnd_Item = {0, 0, ".?AVUICashShopWnd_Item@@"};
static const struct ReferenceType type_UINewSelectCharWnd = {0, 0, ".?AVUINewSelectCharWnd@@"};
static const struct ReferenceType type_UIHairBitmap = {0, 0, ".?AVUIHairBitmap@@"};
static const struct ReferenceType type_UIHairStyleButton = {0, 0, ".?AVUIHairStyleButton@@"};
static const struct ReferenceType type_UINewMakeCharWnd = {0, 0, ".?AVUINewMakeCharWnd@@"};
static const struct ReferenceType type_UICheckAttendanceWnd_Banner = {0, 0, ".?AVUICheckAttendanceWnd_Banner@@"};
static const struct ReferenceType type_UICheckAttendanceWnd = {0, 0, ".?AVUICheckAttendanceWnd@@"};
static const struct ReferenceType type_CChoicePackageItemBoxWnd = {0, 0, ".?AVCChoicePackageItemBoxWnd@@"};
static const struct ReferenceType type_UIControlRectWnd = {0, 0, ".?AVUIControlRectWnd@@"};
static const struct ReferenceType type_UIConnectTwitterWnd = {0, 0, ".?AVUIConnectTwitterWnd@@"};
static const struct ReferenceType type_UIConnectSnsWnd = {0, 0, ".?AVUIConnectSnsWnd@@"};
static const struct ReferenceType type_UITwitterScrollBar = {0, 0, ".?AVUITwitterScrollBar@@"};
static const struct ReferenceType type_UITwitterEditCtrl3 = {0, 0, ".?AVUITwitterEditCtrl3@@"};
static const struct ReferenceType type_UIGradeEnchant_BtnStart = {0, 0, ".?AVUIGradeEnchant_BtnStart@@"};
static const struct ReferenceType type_UIGradeEnchant_BtnEnhancedUp = {0, 0, ".?AVUIGradeEnchant_BtnEnhancedUp@@"};
static const struct ReferenceType type_UIGradeEnchant_BtnEnhancedDown = {0, 0, ".?AVUIGradeEnchant_BtnEnhancedDown@@"};
static const struct ReferenceType type_UIGradeEnchantWnd = {0, 0, ".?AVUIGradeEnchantWnd@@"};
static const struct ReferenceType type_UICaptchaRegisterWnd = {0, 0, ".?AVUICaptchaRegisterWnd@@"};
static const struct ReferenceType type_UIMacroDetectorWnd = {0, 0, ".?AVUIMacroDetectorWnd@@"};
static const struct ReferenceType type_UICaptchaPreviewWnd = {0, 0, ".?AVUICaptchaPreviewWnd@@"};
static const struct ReferenceType type_UIMacroReporterWnd = {0, 0, ".?AVUIMacroReporterWnd@@"};
static const struct ReferenceType type_UIMacroUserInfoWnd = {0, 0, ".?AVUIMacroUserInfoWnd@@"};
static const struct ReferenceType type_UIPreviewEquipWnd = {0, 0, ".?AVUIPreviewEquipWnd@@"};
static const struct ReferenceType type_UIRefining_BtnRefining = {0, 0, ".?AVUIRefining_BtnRefining@@"};
static const struct ReferenceType type_UIRefining_BtnReturn = {0, 0, ".?AVUIRefining_BtnReturn@@"};
static const struct ReferenceType type_UIRefining_BtnAgain = {0, 0, ".?AVUIRefining_BtnAgain@@"};
static const struct ReferenceType type_UIRefining_Notice = {0, 0, ".?AVUIRefining_Notice@@"};
static const struct ReferenceType type_UIRefiningWnd = {0, 0, ".?AVUIRefiningWnd@@"};
static const struct ReferenceType type_UINpcSprWnd = {0, 0, ".?AVUINpcSprWnd@@"};
static const struct ReferenceType type_UIOngoingQuestScrollWnd = {0, 0, ".?AVUIOngoingQuestScrollWnd@@"};
static const struct ReferenceType type_UIRecommededQuestScrollWnd = {0, 0, ".?AVUIRecommededQuestScrollWnd@@"};
static const struct ReferenceType type_UIRenewDetailQuestInfoWnd = {0, 0, ".?AVUIRenewDetailQuestInfoWnd@@"};
static const struct ReferenceType type_UIRenewQuestScrollBar = {0, 0, ".?AVUIRenewQuestScrollBar@@"};
static const struct ReferenceType type_UIRenewQuestWnd = {0, 0, ".?AVUIRenewQuestWnd@@"};
static const struct ReferenceType type_UIRodexScrollBar = {0, 0, ".?AVUIRodexScrollBar@@"};
static const struct ReferenceType type_UIRodexEditCtrl3 = {0, 0, ".?AVUIRodexEditCtrl3@@"};
static const struct ReferenceType type_UIRodexComposeWnd = {0, 0, ".?AVUIRodexComposeWnd@@"};
static const struct ReferenceType type_UIRodexContentsWnd = {0, 0, ".?AVUIRodexContentsWnd@@"};
static const struct ReferenceType type_UIRodexInboxWnd = {0, 0, ".?AVUIRodexInboxWnd@@"};
static const struct ReferenceType type_UIRoulletteWnd = {0, 0, ".?AVUIRoulletteWnd@@"};
static const struct ReferenceType type_UIRenderTargetWnd = {0, 0, ".?AVUIRenderTargetWnd@@"};
static const struct ReferenceType type_CIcon = {0, 0, ".?AVCIcon@@"};
static const struct ReferenceType type_CBmpObjWnd = {0, 0, ".?AVCBmpObjWnd@@"};
static const struct ReferenceType type_CSignBoardWnd = {0, 0, ".?AVCSignBoardWnd@@"};
static const struct ReferenceType type_C3DCooRendering = {0, 0, ".?AVC3DCooRendering@@"};
static const struct ReferenceType type_UIDisplayCharacter = {0, 0, ".?AVUIDisplayCharacter@@"};
static const struct ReferenceType type_UIStylingShopButton = {0, 0, ".?AVUIStylingShopButton@@"};
static const struct ReferenceType type_UIAccBitmapButton = {0, 0, ".?AVUIAccBitmapButton@@"};
static const struct ReferenceType type_UI2ndCostumeCheckBox = {0, 0, ".?AVUI2ndCostumeCheckBox@@"};
static const struct ReferenceType type_UIShowInfoWnd = {0, 0, ".?AVUIShowInfoWnd@@"};
static const struct ReferenceType type_UIStylingshopTabParentWnd = {0, 0, ".?AVUIStylingshopTabParentWnd@@"};
static const struct ReferenceType type_UIBasicStylingWnd = {0, 0, ".?AVUIBasicStylingWnd@@"};
static const struct ReferenceType type_UIAccessoryCusWnd = {0, 0, ".?AVUIAccessoryCusWnd@@"};
static const struct ReferenceType type_UIAccessoryShowCase = {0, 0, ".?AVUIAccessoryShowCase@@"};
static const struct ReferenceType type_UI2ndCostumeWnd = {0, 0, ".?AVUI2ndCostumeWnd@@"};
static const struct ReferenceType type_UISlotOfMyStyleWnd = {0, 0, ".?AVUISlotOfMyStyleWnd@@"};
static const struct ReferenceType type_UISlotForSelectWnd = {0, 0, ".?AVUISlotForSelectWnd@@"};
static const struct ReferenceType type_UIMyStyleWnd = {0, 0, ".?AVUIMyStyleWnd@@"};
static const struct ReferenceType type_UIStylingShopWnd = {0, 0, ".?AVUIStylingShopWnd@@"};
static const struct ReferenceType type_UISwapEquipmentWnd = {0, 0, ".?AVUISwapEquipmentWnd@@"};
static const struct ReferenceType type_UISwapBitmapButton = {0, 0, ".?AVUISwapBitmapButton@@"};
static const struct ReferenceType type_LineCutter_UIText = {0, 0, ".?AVUIText@LineCutter@@"};
static const struct ReferenceType type_TagTextViewer_UIScroll = {0, 0, ".?AVUIScroll@TagTextViewer@@"};
static const struct ReferenceType type_UITagTextViewer = {0, 0, ".?AVUITagTextViewer@@"};
static const struct ReferenceType type_UIBarGraphPlayerAP = {0, 0, ".?AVUIBarGraphPlayerAP@@"};
static const struct ReferenceType type_UIBasicInfoWnd = {0, 0, ".?AVUIBasicInfoWnd@@"};
static const struct ReferenceType type_UINewSkillListWnd = {0, 0, ".?AVUINewSkillListWnd@@"};
static const struct ReferenceType type_UISkipButton = {0, 0, ".?AVUISkipButton@@"};
static const struct ReferenceType type_UIVideoWnd = {0, 0, ".?AVUIVideoWnd@@"};
static const struct ReferenceType type_UIEntryQueueHelpWnd = {0, 0, ".?AVUIEntryQueueHelpWnd@@"};
static const struct ReferenceType type_UIEntryQueueListBox = {0, 0, ".?AVUIEntryQueueListBox@@"};
static const struct ReferenceType type_UIEntryQueueRequestWnd = {0, 0, ".?AVUIEntryQueueRequestWnd@@"};
extern const uintptr_t locator_CRes[];
extern const uintptr_t hierarchy_CRes[];
extern const uintptr_t bases_CRes[];
extern const uintptr_t base_CRes[];
extern const uintptr_t locator_CCharacterNpc[];
extern const uintptr_t hierarchy_CCharacterNpc[];
extern const uintptr_t bases_CCharacterNpc[];
extern const uintptr_t base_CCharacterNpc[];
extern const uintptr_t base_CNpc[];
extern const uintptr_t hierarchy_CNpc[];
extern const uintptr_t bases_CNpc[];
extern const uintptr_t base_CGameActor[];
extern const uintptr_t hierarchy_CGameActor[];
extern const uintptr_t bases_CGameActor[];
extern const uintptr_t base_CAbleToMakeEffect[];
extern const uintptr_t hierarchy_CAbleToMakeEffect[];
extern const uintptr_t bases_CAbleToMakeEffect[];
extern const uintptr_t base_CRenderObject[];
extern const uintptr_t hierarchy_CRenderObject[];
extern const uintptr_t bases_CRenderObject[];
extern const uintptr_t base_CGameObject[];
extern const uintptr_t hierarchy_CGameObject[];
extern const uintptr_t bases_CGameObject[];
extern const uintptr_t base_UIEditCtrl[];
extern const uintptr_t hierarchy_UIEditCtrl[];
extern const uintptr_t bases_UIEditCtrl[];
extern const uintptr_t base_UIWindow[];
extern const uintptr_t hierarchy_UIWindow[];
extern const uintptr_t bases_UIWindow[];
extern const uintptr_t base_UIRPData[];
extern const uintptr_t hierarchy_UIRPData[];
extern const uintptr_t bases_UIRPData[];
extern const uintptr_t locator_UIBitmapEditCtrl[];
extern const uintptr_t hierarchy_UIBitmapEditCtrl[];
extern const uintptr_t bases_UIBitmapEditCtrl[];
extern const uintptr_t base_UIBitmapEditCtrl[];
extern const uintptr_t base_UIFrameWnd[];
extern const uintptr_t hierarchy_UIFrameWnd[];
extern const uintptr_t bases_UIFrameWnd[];
extern const uintptr_t locator_UIListBox_NaviSearch[];
extern const uintptr_t hierarchy_UIListBox_NaviSearch[];
extern const uintptr_t bases_UIListBox_NaviSearch[];
extern const uintptr_t base_UIListBox_NaviSearch[];
extern const uintptr_t base_UIListBox[];
extern const uintptr_t hierarchy_UIListBox[];
extern const uintptr_t bases_UIListBox[];
extern const uintptr_t locator_UINavigationroadiconWnd[];
extern const uintptr_t hierarchy_UINavigationroadiconWnd[];
extern const uintptr_t bases_UINavigationroadiconWnd[];
extern const uintptr_t base_UINavigationroadiconWnd[];
extern const uintptr_t locator_UINavigationRuideWnd[];
extern const uintptr_t hierarchy_UINavigationRuideWnd[];
extern const uintptr_t bases_UINavigationRuideWnd[];
extern const uintptr_t base_UINavigationRuideWnd[];
extern const uintptr_t locator_UIPrivateAirplaneWnd[];
extern const uintptr_t hierarchy_UIPrivateAirplaneWnd[];
extern const uintptr_t bases_UIPrivateAirplaneWnd[];
extern const uintptr_t base_UIPrivateAirplaneWnd[];
extern const uintptr_t locator_UINavigationV4Wnd[];
extern const uintptr_t hierarchy_UINavigationV4Wnd[];
extern const uintptr_t bases_UINavigationV4Wnd[];
extern const uintptr_t base_UINavigationV4Wnd[];
extern const uintptr_t locator_CRollingCutterRagEffect[];
extern const uintptr_t hierarchy_CRollingCutterRagEffect[];
extern const uintptr_t bases_CRollingCutterRagEffect[];
extern const uintptr_t base_CRollingCutterRagEffect[];
extern const uintptr_t base_CRagEffect[];
extern const uintptr_t hierarchy_CRagEffect[];
extern const uintptr_t bases_CRagEffect[];
extern const uintptr_t locator_UIEditCtrl[];
extern const uintptr_t locator_UIEditCtrl2[];
extern const uintptr_t hierarchy_UIEditCtrl2[];
extern const uintptr_t bases_UIEditCtrl2[];
extern const uintptr_t base_UIEditCtrl2[];
extern const uintptr_t locator_UIEditCtrl4[];
extern const uintptr_t hierarchy_UIEditCtrl4[];
extern const uintptr_t bases_UIEditCtrl4[];
extern const uintptr_t base_UIEditCtrl4[];
extern const uintptr_t locator_UIEditCtrl5[];
extern const uintptr_t hierarchy_UIEditCtrl5[];
extern const uintptr_t bases_UIEditCtrl5[];
extern const uintptr_t base_UIEditCtrl5[];
extern const uintptr_t locator_UIEditCtrl3[];
extern const uintptr_t hierarchy_UIEditCtrl3[];
extern const uintptr_t bases_UIEditCtrl3[];
extern const uintptr_t base_UIEditCtrl3[];
extern const uintptr_t locator_UIChatEditCtrl[];
extern const uintptr_t hierarchy_UIChatEditCtrl[];
extern const uintptr_t bases_UIChatEditCtrl[];
extern const uintptr_t base_UIChatEditCtrl[];
extern const uintptr_t locator_UIWhisperEditCtrl[];
extern const uintptr_t hierarchy_UIWhisperEditCtrl[];
extern const uintptr_t bases_UIWhisperEditCtrl[];
extern const uintptr_t base_UIWhisperEditCtrl[];
extern const uintptr_t locator_UIEditCtrl_OldList[];
extern const uintptr_t hierarchy_UIEditCtrl_OldList[];
extern const uintptr_t bases_UIEditCtrl_OldList[];
extern const uintptr_t base_UIEditCtrl_OldList[];
extern const uintptr_t locator_UIStaticText[];
extern const uintptr_t hierarchy_UIStaticText[];
extern const uintptr_t bases_UIStaticText[];
extern const uintptr_t base_UIStaticText[];
extern const uintptr_t locator_UIStaticText3[];
extern const uintptr_t hierarchy_UIStaticText3[];
extern const uintptr_t bases_UIStaticText3[];
extern const uintptr_t base_UIStaticText3[];
extern const uintptr_t locator_UIBalloonText[];
extern const uintptr_t hierarchy_UIBalloonText[];
extern const uintptr_t bases_UIBalloonText[];
extern const uintptr_t base_UIBalloonText[];
extern const uintptr_t locator_UINameBalloonText[];
extern const uintptr_t hierarchy_UINameBalloonText[];
extern const uintptr_t bases_UINameBalloonText[];
extern const uintptr_t base_UINameBalloonText[];
extern const uintptr_t locator_UITransBalloonText[];
extern const uintptr_t hierarchy_UITransBalloonText[];
extern const uintptr_t bases_UITransBalloonText[];
extern const uintptr_t base_UITransBalloonText[];
extern const uintptr_t locator_UIAchBalloonText[];
extern const uintptr_t hierarchy_UIAchBalloonText[];
extern const uintptr_t bases_UIAchBalloonText[];
extern const uintptr_t base_UIAchBalloonText[];
extern const uintptr_t locator_UIButton[];
extern const uintptr_t hierarchy_UIButton[];
extern const uintptr_t bases_UIButton[];
extern const uintptr_t base_UIButton[];
extern const uintptr_t locator_UINoneVisibleButton[];
extern const uintptr_t hierarchy_UINoneVisibleButton[];
extern const uintptr_t bases_UINoneVisibleButton[];
extern const uintptr_t base_UINoneVisibleButton[];
extern const uintptr_t locator_UIColorButton[];
extern const uintptr_t hierarchy_UIColorButton[];
extern const uintptr_t bases_UIColorButton[];
extern const uintptr_t base_UIColorButton[];
extern const uintptr_t locator_UIBitmapButton[];
extern const uintptr_t hierarchy_UIBitmapButton[];
extern const uintptr_t bases_UIBitmapButton[];
extern const uintptr_t base_UIBitmapButton[];
extern const uintptr_t locator_UIBasicButton[];
extern const uintptr_t hierarchy_UIBasicButton[];
extern const uintptr_t bases_UIBasicButton[];
extern const uintptr_t base_UIBasicButton[];
extern const uintptr_t locator_UIBmpTextButton[];
extern const uintptr_t hierarchy_UIBmpTextButton[];
extern const uintptr_t bases_UIBmpTextButton[];
extern const uintptr_t base_UIBmpTextButton[];
extern const uintptr_t locator_UIBmpTextButton2[];
extern const uintptr_t hierarchy_UIBmpTextButton2[];
extern const uintptr_t bases_UIBmpTextButton2[];
extern const uintptr_t base_UIBmpTextButton2[];
extern const uintptr_t locator_UITextureButton[];
extern const uintptr_t hierarchy_UITextureButton[];
extern const uintptr_t bases_UITextureButton[];
extern const uintptr_t base_UITextureButton[];
extern const uintptr_t locator_UISpriteButton[];
extern const uintptr_t hierarchy_UISpriteButton[];
extern const uintptr_t bases_UISpriteButton[];
extern const uintptr_t base_UISpriteButton[];
extern const uintptr_t locator_UIHeadsprButton[];
extern const uintptr_t hierarchy_UIHeadsprButton[];
extern const uintptr_t bases_UIHeadsprButton[];
extern const uintptr_t base_UIHeadsprButton[];
extern const uintptr_t locator_UICheckBox[];
extern const uintptr_t hierarchy_UICheckBox[];
extern const uintptr_t bases_UICheckBox[];
extern const uintptr_t base_UICheckBox[];
extern const uintptr_t locator_UIStaticBitmap[];
extern const uintptr_t hierarchy_UIStaticBitmap[];
extern const uintptr_t bases_UIStaticBitmap[];
extern const uintptr_t base_UIStaticBitmap[];
extern const uintptr_t locator_UISlotBitmap[];
extern const uintptr_t hierarchy_UISlotBitmap[];
extern const uintptr_t bases_UISlotBitmap[];
extern const uintptr_t base_UISlotBitmap[];
extern const uintptr_t locator_UIRadioBtn[];
extern const uintptr_t hierarchy_UIRadioBtn[];
extern const uintptr_t bases_UIRadioBtn[];
extern const uintptr_t base_UIRadioBtn[];
extern const uintptr_t locator_UIResizer[];
extern const uintptr_t hierarchy_UIResizer[];
extern const uintptr_t bases_UIResizer[];
extern const uintptr_t base_UIResizer[];
extern const uintptr_t locator_UITab[];
extern const uintptr_t hierarchy_UITab[];
extern const uintptr_t bases_UITab[];
extern const uintptr_t base_UITab[];
extern const uintptr_t locator_UIBitmapTab[];
extern const uintptr_t hierarchy_UIBitmapTab[];
extern const uintptr_t bases_UIBitmapTab[];
extern const uintptr_t base_UIBitmapTab[];
extern const uintptr_t locator_UIChatRoomTitle[];
extern const uintptr_t hierarchy_UIChatRoomTitle[];
extern const uintptr_t bases_UIChatRoomTitle[];
extern const uintptr_t base_UIChatRoomTitle[];
extern const uintptr_t locator_UIMerchantShopTitle[];
extern const uintptr_t hierarchy_UIMerchantShopTitle[];
extern const uintptr_t bases_UIMerchantShopTitle[];
extern const uintptr_t base_UIMerchantShopTitle[];
extern const uintptr_t locator_UIEmblemWnd[];
extern const uintptr_t hierarchy_UIEmblemWnd[];
extern const uintptr_t bases_UIEmblemWnd[];
extern const uintptr_t base_UIEmblemWnd[];
extern const uintptr_t locator_UIWordDisplayWnd[];
extern const uintptr_t hierarchy_UIWordDisplayWnd[];
extern const uintptr_t bases_UIWordDisplayWnd[];
extern const uintptr_t base_UIWordDisplayWnd[];
extern const uintptr_t locator_UIScrollBar[];
extern const uintptr_t hierarchy_UIScrollBar[];
extern const uintptr_t bases_UIScrollBar[];
extern const uintptr_t base_UIScrollBar[];
extern const uintptr_t locator_UIScrollBar2[];
extern const uintptr_t hierarchy_UIScrollBar2[];
extern const uintptr_t bases_UIScrollBar2[];
extern const uintptr_t base_UIScrollBar2[];
extern const uintptr_t locator_UIScrollBar3[];
extern const uintptr_t hierarchy_UIScrollBar3[];
extern const uintptr_t bases_UIScrollBar3[];
extern const uintptr_t base_UIScrollBar3[];
extern const uintptr_t locator_UIScrollBar4[];
extern const uintptr_t hierarchy_UIScrollBar4[];
extern const uintptr_t bases_UIScrollBar4[];
extern const uintptr_t base_UIScrollBar4[];
extern const uintptr_t locator_UIScrollBar_Custom[];
extern const uintptr_t hierarchy_UIScrollBar_Custom[];
extern const uintptr_t bases_UIScrollBar_Custom[];
extern const uintptr_t base_UIScrollBar_Custom[];
extern const uintptr_t locator_UIZeroQuestScrollBar[];
extern const uintptr_t hierarchy_UIZeroQuestScrollBar[];
extern const uintptr_t bases_UIZeroQuestScrollBar[];
extern const uintptr_t base_UIZeroQuestScrollBar[];
extern const uintptr_t locator_UITextButton[];
extern const uintptr_t hierarchy_UITextButton[];
extern const uintptr_t bases_UITextButton[];
extern const uintptr_t base_UITextButton[];
extern const uintptr_t locator_UIItemTagButton[];
extern const uintptr_t hierarchy_UIItemTagButton[];
extern const uintptr_t bases_UIItemTagButton[];
extern const uintptr_t base_UIItemTagButton[];
extern const uintptr_t locator_UIEditCtrlVarId[];
extern const uintptr_t hierarchy_UIEditCtrlVarId[];
extern const uintptr_t bases_UIEditCtrlVarId[];
extern const uintptr_t base_UIEditCtrlVarId[];
extern const uintptr_t locator_UIItemTagOnChat[];
extern const uintptr_t hierarchy_UIItemTagOnChat[];
extern const uintptr_t bases_UIItemTagOnChat[];
extern const uintptr_t base_UIItemTagOnChat[];
extern const uintptr_t base_UIItemTagCtrl[];
extern const uintptr_t hierarchy_UIItemTagCtrl[];
extern const uintptr_t bases_UIItemTagCtrl[];
extern const uintptr_t locator_UIBarGraphPlayer[];
extern const uintptr_t hierarchy_UIBarGraphPlayer[];
extern const uintptr_t bases_UIBarGraphPlayer[];
extern const uintptr_t base_UIBarGraphPlayer[];
extern const uintptr_t locator_UIBarGraphHomun[];
extern const uintptr_t hierarchy_UIBarGraphHomun[];
extern const uintptr_t bases_UIBarGraphHomun[];
extern const uintptr_t base_UIBarGraphHomun[];
extern const uintptr_t locator_UIBarGraph[];
extern const uintptr_t hierarchy_UIBarGraph[];
extern const uintptr_t bases_UIBarGraph[];
extern const uintptr_t base_UIBarGraph[];
extern const uintptr_t locator_UIBarGraph2[];
extern const uintptr_t hierarchy_UIBarGraph2[];
extern const uintptr_t bases_UIBarGraph2[];
extern const uintptr_t base_UIBarGraph2[];
extern const uintptr_t locator_UIBarGraph3[];
extern const uintptr_t hierarchy_UIBarGraph3[];
extern const uintptr_t bases_UIBarGraph3[];
extern const uintptr_t base_UIBarGraph3[];
extern const uintptr_t locator_UIINT64BarGraph[];
extern const uintptr_t hierarchy_UIINT64BarGraph[];
extern const uintptr_t bases_UIINT64BarGraph[];
extern const uintptr_t base_UIINT64BarGraph[];
extern const uintptr_t locator_UIPetInfoBarGraph[];
extern const uintptr_t hierarchy_UIPetInfoBarGraph[];
extern const uintptr_t bases_UIPetInfoBarGraph[];
extern const uintptr_t base_UIPetInfoBarGraph[];
extern const uintptr_t locator_UIGage[];
extern const uintptr_t hierarchy_UIGage[];
extern const uintptr_t bases_UIGage[];
extern const uintptr_t base_UIGage[];
extern const uintptr_t locator_UIRechargeGage[];
extern const uintptr_t hierarchy_UIRechargeGage[];
extern const uintptr_t bases_UIRechargeGage[];
extern const uintptr_t base_UIRechargeGage[];
extern const uintptr_t locator_UIPcGage[];
extern const uintptr_t hierarchy_UIPcGage[];
extern const uintptr_t bases_UIPcGage[];
extern const uintptr_t base_UIPcGage[];
extern const uintptr_t locator_UIPlayerGage[];
extern const uintptr_t hierarchy_UIPlayerGage[];
extern const uintptr_t bases_UIPlayerGage[];
extern const uintptr_t base_UIPlayerGage[];
extern const uintptr_t locator_UIMonsterGage[];
extern const uintptr_t hierarchy_UIMonsterGage[];
extern const uintptr_t bases_UIMonsterGage[];
extern const uintptr_t base_UIMonsterGage[];
extern const uintptr_t locator_UIColorTransBalloonText[];
extern const uintptr_t hierarchy_UIColorTransBalloonText[];
extern const uintptr_t bases_UIColorTransBalloonText[];
extern const uintptr_t base_UIColorTransBalloonText[];
extern const uintptr_t locator_UIBitmapButton2[];
extern const uintptr_t hierarchy_UIBitmapButton2[];
extern const uintptr_t bases_UIBitmapButton2[];
extern const uintptr_t base_UIBitmapButton2[];
extern const uintptr_t locator_UIListBox[];
extern const uintptr_t locator_UIPackageBoxNameList[];
extern const uintptr_t hierarchy_UIPackageBoxNameList[];
extern const uintptr_t bases_UIPackageBoxNameList[];
extern const uintptr_t base_UIPackageBoxNameList[];
extern const uintptr_t locator_UIListBox2[];
extern const uintptr_t hierarchy_UIListBox2[];
extern const uintptr_t bases_UIListBox2[];
extern const uintptr_t base_UIListBox2[];
extern const uintptr_t locator_UIListBoxID[];
extern const uintptr_t hierarchy_UIListBoxID[];
extern const uintptr_t bases_UIListBoxID[];
extern const uintptr_t base_UIListBoxID[];
extern const uintptr_t locator_UIListBoxNavi[];
extern const uintptr_t hierarchy_UIListBoxNavi[];
extern const uintptr_t bases_UIListBoxNavi[];
extern const uintptr_t base_UIListBoxNavi[];
extern const uintptr_t locator_UIListBoxEx[];
extern const uintptr_t hierarchy_UIListBoxEx[];
extern const uintptr_t bases_UIListBoxEx[];
extern const uintptr_t base_UIListBoxEx[];
extern const uintptr_t locator_UIListBoxTitle[];
extern const uintptr_t hierarchy_UIListBoxTitle[];
extern const uintptr_t bases_UIListBoxTitle[];
extern const uintptr_t base_UIListBoxTitle[];
extern const uintptr_t locator_UIComboBoxEx[];
extern const uintptr_t hierarchy_UIComboBoxEx[];
extern const uintptr_t bases_UIComboBoxEx[];
extern const uintptr_t base_UIComboBoxEx[];
extern const uintptr_t locator_UIComboBoxEx2[];
extern const uintptr_t hierarchy_UIComboBoxEx2[];
extern const uintptr_t bases_UIComboBoxEx2[];
extern const uintptr_t base_UIComboBoxEx2[];
extern const uintptr_t locator_UIItemStoreComboBox[];
extern const uintptr_t hierarchy_UIItemStoreComboBox[];
extern const uintptr_t bases_UIItemStoreComboBox[];
extern const uintptr_t base_UIItemStoreComboBox[];
extern const uintptr_t locator_UIComboBox[];
extern const uintptr_t hierarchy_UIComboBox[];
extern const uintptr_t bases_UIComboBox[];
extern const uintptr_t base_UIComboBox[];
extern const uintptr_t locator_UIComboBoxID[];
extern const uintptr_t hierarchy_UIComboBoxID[];
extern const uintptr_t bases_UIComboBoxID[];
extern const uintptr_t base_UIComboBoxID[];
extern const uintptr_t locator_UIComboBoxDefWnd[];
extern const uintptr_t hierarchy_UIComboBoxDefWnd[];
extern const uintptr_t bases_UIComboBoxDefWnd[];
extern const uintptr_t base_UIComboBoxDefWnd[];
extern const uintptr_t locator_UIComboBoxDef2Wnd[];
extern const uintptr_t hierarchy_UIComboBoxDef2Wnd[];
extern const uintptr_t bases_UIComboBoxDef2Wnd[];
extern const uintptr_t base_UIComboBoxDef2Wnd[];
extern const uintptr_t locator_UIItemStoreComboBoxWnd[];
extern const uintptr_t hierarchy_UIItemStoreComboBoxWnd[];
extern const uintptr_t bases_UIItemStoreComboBoxWnd[];
extern const uintptr_t base_UIItemStoreComboBoxWnd[];
extern const uintptr_t locator_UIComboBox_BmpWnd[];
extern const uintptr_t hierarchy_UIComboBox_BmpWnd[];
extern const uintptr_t bases_UIComboBox_BmpWnd[];
extern const uintptr_t base_UIComboBox_BmpWnd[];
extern const uintptr_t locator_UIComboBoxV2Wnd[];
extern const uintptr_t hierarchy_UIComboBoxV2Wnd[];
extern const uintptr_t bases_UIComboBoxV2Wnd[];
extern const uintptr_t base_UIComboBoxV2Wnd[];
extern const uintptr_t locator_TableBox[];
extern const uintptr_t hierarchy_TableBox[];
extern const uintptr_t bases_TableBox[];
extern const uintptr_t base_TableBox[];
extern const uintptr_t locator_UITableBox[];
extern const uintptr_t hierarchy_UITableBox[];
extern const uintptr_t bases_UITableBox[];
extern const uintptr_t base_UITableBox[];
extern const uintptr_t locator_UIChatHisBox[];
extern const uintptr_t hierarchy_UIChatHisBox[];
extern const uintptr_t bases_UIChatHisBox[];
extern const uintptr_t base_UIChatHisBox[];
extern const uintptr_t locator_UIChatHisBox2[];
extern const uintptr_t hierarchy_UIChatHisBox2[];
extern const uintptr_t bases_UIChatHisBox2[];
extern const uintptr_t base_UIChatHisBox2[];
extern const uintptr_t locator_UIChatHisBox3[];
extern const uintptr_t hierarchy_UIChatHisBox3[];
extern const uintptr_t bases_UIChatHisBox3[];
extern const uintptr_t base_UIChatHisBox3[];
extern const uintptr_t locator_UIMemberListBox[];
extern const uintptr_t hierarchy_UIMemberListBox[];
extern const uintptr_t bases_UIMemberListBox[];
extern const uintptr_t base_UIMemberListBox[];
extern const uintptr_t locator_UITextViewer[];
extern const uintptr_t hierarchy_UITextViewer[];
extern const uintptr_t bases_UITextViewer[];
extern const uintptr_t base_UITextViewer[];
extern const uintptr_t locator_UIString[];
extern const uintptr_t hierarchy_UIString[];
extern const uintptr_t bases_UIString[];
extern const uintptr_t base_UIString[];
extern const uintptr_t locator_UITextViewEX[];
extern const uintptr_t hierarchy_UITextViewEX[];
extern const uintptr_t bases_UITextViewEX[];
extern const uintptr_t base_UITextViewEX[];
extern const uintptr_t locator_UIServerListBox[];
extern const uintptr_t hierarchy_UIServerListBox[];
extern const uintptr_t bases_UIServerListBox[];
extern const uintptr_t base_UIServerListBox[];
extern const uintptr_t locator_UIImgMenu[];
extern const uintptr_t hierarchy_UIImgMenu[];
extern const uintptr_t bases_UIImgMenu[];
extern const uintptr_t base_UIImgMenu[];
extern const uintptr_t locator_UITVTextButton[];
extern const uintptr_t hierarchy_UITVTextButton[];
extern const uintptr_t bases_UITVTextButton[];
extern const uintptr_t base_UITVTextButton[];
extern const uintptr_t locator_UITVBitmapButton[];
extern const uintptr_t hierarchy_UITVBitmapButton[];
extern const uintptr_t bases_UITVBitmapButton[];
extern const uintptr_t base_UITVBitmapButton[];
extern const uintptr_t locator_UITreeView[];
extern const uintptr_t hierarchy_UITreeView[];
extern const uintptr_t bases_UITreeView[];
extern const uintptr_t base_UITreeView[];
extern const uintptr_t locator_UITabControl[];
extern const uintptr_t hierarchy_UITabControl[];
extern const uintptr_t bases_UITabControl[];
extern const uintptr_t base_UITabControl[];
extern const uintptr_t locator_UIShowAndEditTimeCtrl[];
extern const uintptr_t hierarchy_UIShowAndEditTimeCtrl[];
extern const uintptr_t bases_UIShowAndEditTimeCtrl[];
extern const uintptr_t base_UIShowAndEditTimeCtrl[];
extern const uintptr_t locator_UIBasicItemCtrl[];
extern const uintptr_t hierarchy_UIBasicItemCtrl[];
extern const uintptr_t bases_UIBasicItemCtrl[];
extern const uintptr_t base_UIBasicItemCtrl[];
extern const uintptr_t locator_UIBasicSkillCtrl[];
extern const uintptr_t hierarchy_UIBasicSkillCtrl[];
extern const uintptr_t bases_UIBasicSkillCtrl[];
extern const uintptr_t base_UIBasicSkillCtrl[];
extern const uintptr_t locator_UINewComboBoxCtrl[];
extern const uintptr_t hierarchy_UINewComboBoxCtrl[];
extern const uintptr_t bases_UINewComboBoxCtrl[];
extern const uintptr_t base_UINewComboBoxCtrl[];
extern const uintptr_t locator_UIItemMoveInfo[];
extern const uintptr_t hierarchy_UIItemMoveInfo[];
extern const uintptr_t bases_UIItemMoveInfo[];
extern const uintptr_t base_UIItemMoveInfo[];
extern const uintptr_t locator_UIItemTagCtrl[];
extern const uintptr_t locator_UIItemTagOnMegaPhone[];
extern const uintptr_t hierarchy_UIItemTagOnMegaPhone[];
extern const uintptr_t bases_UIItemTagOnMegaPhone[];
extern const uintptr_t base_UIItemTagOnMegaPhone[];
extern const uintptr_t locator_UIItemTagOnMail[];
extern const uintptr_t hierarchy_UIItemTagOnMail[];
extern const uintptr_t bases_UIItemTagOnMail[];
extern const uintptr_t base_UIItemTagOnMail[];
extern const uintptr_t locator_UIBmpRadioBtn[];
extern const uintptr_t hierarchy_UIBmpRadioBtn[];
extern const uintptr_t bases_UIBmpRadioBtn[];
extern const uintptr_t base_UIBmpRadioBtn[];
extern const uintptr_t locator_UIEditCtrl_Custom[];
extern const uintptr_t hierarchy_UIEditCtrl_Custom[];
extern const uintptr_t bases_UIEditCtrl_Custom[];
extern const uintptr_t base_UIEditCtrl_Custom[];
extern const uintptr_t locator_UIEditCtrl_Currency[];
extern const uintptr_t hierarchy_UIEditCtrl_Currency[];
extern const uintptr_t bases_UIEditCtrl_Currency[];
extern const uintptr_t base_UIEditCtrl_Currency[];
extern const uintptr_t locator_UICaptchaImage[];
extern const uintptr_t hierarchy_UICaptchaImage[];
extern const uintptr_t bases_UICaptchaImage[];
extern const uintptr_t base_UICaptchaImage[];
extern const uintptr_t locator_UIStaticItem[];
extern const uintptr_t hierarchy_UIStaticItem[];
extern const uintptr_t bases_UIStaticItem[];
extern const uintptr_t base_UIStaticItem[];
extern const uintptr_t locator_UIStaticItem_Illust[];
extern const uintptr_t hierarchy_UIStaticItem_Illust[];
extern const uintptr_t bases_UIStaticItem_Illust[];
extern const uintptr_t base_UIStaticItem_Illust[];
extern const uintptr_t locator_UINaviLinkButton[];
extern const uintptr_t hierarchy_UINaviLinkButton[];
extern const uintptr_t bases_UINaviLinkButton[];
extern const uintptr_t base_UINaviLinkButton[];
extern const uintptr_t locator_UISlotBmpButton[];
extern const uintptr_t hierarchy_UISlotBmpButton[];
extern const uintptr_t bases_UISlotBmpButton[];
extern const uintptr_t base_UISlotBmpButton[];
extern const uintptr_t locator_UIDescriptRace[];
extern const uintptr_t hierarchy_UIDescriptRace[];
extern const uintptr_t bases_UIDescriptRace[];
extern const uintptr_t base_UIDescriptRace[];
extern const uintptr_t locator_UIRPImageWnd[];
extern const uintptr_t hierarchy_UIRPImageWnd[];
extern const uintptr_t bases_UIRPImageWnd[];
extern const uintptr_t base_UIRPImageWnd[];
extern const uintptr_t locator_UIScrollBar_Navi[];
extern const uintptr_t hierarchy_UIScrollBar_Navi[];
extern const uintptr_t bases_UIScrollBar_Navi[];
extern const uintptr_t base_UIScrollBar_Navi[];
extern const uintptr_t locator_UIWaitWnd[];
extern const uintptr_t hierarchy_UIWaitWnd[];
extern const uintptr_t bases_UIWaitWnd[];
extern const uintptr_t base_UIWaitWnd[];
extern const uintptr_t locator_UIExchangeWnd[];
extern const uintptr_t hierarchy_UIExchangeWnd[];
extern const uintptr_t bases_UIExchangeWnd[];
extern const uintptr_t base_UIExchangeWnd[];
extern const uintptr_t locator_UILoadingWnd[];
extern const uintptr_t hierarchy_UILoadingWnd[];
extern const uintptr_t bases_UILoadingWnd[];
extern const uintptr_t base_UILoadingWnd[];
extern const uintptr_t locator_UIToolTipWnd[];
extern const uintptr_t hierarchy_UIToolTipWnd[];
extern const uintptr_t bases_UIToolTipWnd[];
extern const uintptr_t base_UIToolTipWnd[];
extern const uintptr_t locator_UIEquipWnd[];
extern const uintptr_t hierarchy_UIEquipWnd[];
extern const uintptr_t bases_UIEquipWnd[];
extern const uintptr_t base_UIEquipWnd[];
extern const uintptr_t locator_UInCashWnd[];
extern const uintptr_t hierarchy_UInCashWnd[];
extern const uintptr_t bases_UInCashWnd[];
extern const uintptr_t base_UInCashWnd[];
extern const uintptr_t base_UIItemBaseWnd[];
extern const uintptr_t hierarchy_UIItemBaseWnd[];
extern const uintptr_t bases_UIItemBaseWnd[];
extern const uintptr_t locator_UIItemParamChangeDisplayWnd[];
extern const uintptr_t hierarchy_UIItemParamChangeDisplayWnd[];
extern const uintptr_t bases_UIItemParamChangeDisplayWnd[];
extern const uintptr_t base_UIItemParamChangeDisplayWnd[];
extern const uintptr_t locator_UIPasswordWnd[];
extern const uintptr_t hierarchy_UIPasswordWnd[];
extern const uintptr_t bases_UIPasswordWnd[];
extern const uintptr_t base_UIPasswordWnd[];
extern const uintptr_t base_UIItemDropCntWnd[];
extern const uintptr_t hierarchy_UIItemDropCntWnd[];
extern const uintptr_t bases_UIItemDropCntWnd[];
extern const uintptr_t locator_UIEmailAddressWnd[];
extern const uintptr_t hierarchy_UIEmailAddressWnd[];
extern const uintptr_t bases_UIEmailAddressWnd[];
extern const uintptr_t base_UIEmailAddressWnd[];
extern const uintptr_t locator_UISkillNameChangeWnd[];
extern const uintptr_t hierarchy_UISkillNameChangeWnd[];
extern const uintptr_t bases_UISkillNameChangeWnd[];
extern const uintptr_t base_UISkillNameChangeWnd[];
extern const uintptr_t locator_UINotifyItemObtainWnd[];
extern const uintptr_t hierarchy_UINotifyItemObtainWnd[];
extern const uintptr_t bases_UINotifyItemObtainWnd[];
extern const uintptr_t base_UINotifyItemObtainWnd[];
extern const uintptr_t locator_UINpcEditDialogWnd[];
extern const uintptr_t hierarchy_UINpcEditDialogWnd[];
extern const uintptr_t bases_UINpcEditDialogWnd[];
extern const uintptr_t base_UINpcEditDialogWnd[];
extern const uintptr_t locator_UINpcTextEditDialogWnd[];
extern const uintptr_t hierarchy_UINpcTextEditDialogWnd[];
extern const uintptr_t bases_UINpcTextEditDialogWnd[];
extern const uintptr_t base_UINpcTextEditDialogWnd[];
extern const uintptr_t locator_UIStatusWnd[];
extern const uintptr_t hierarchy_UIStatusWnd[];
extern const uintptr_t bases_UIStatusWnd[];
extern const uintptr_t base_UIStatusWnd[];
extern const uintptr_t locator_UIItemCollectionWnd[];
extern const uintptr_t hierarchy_UIItemCollectionWnd[];
extern const uintptr_t bases_UIItemCollectionWnd[];
extern const uintptr_t base_UIItemCollectionWnd[];
extern const uintptr_t locator_UICombinedCardItemCollectionWnd[];
extern const uintptr_t hierarchy_UICombinedCardItemCollectionWnd[];
extern const uintptr_t bases_UICombinedCardItemCollectionWnd[];
extern const uintptr_t base_UICombinedCardItemCollectionWnd[];
extern const uintptr_t locator_UIItemCollection_ComparisonWnd[];
extern const uintptr_t hierarchy_UIItemCollection_ComparisonWnd[];
extern const uintptr_t bases_UIItemCollection_ComparisonWnd[];
extern const uintptr_t base_UIItemCollection_ComparisonWnd[];
extern const uintptr_t locator_UICombinedCardItemCollection_ComparisonWnd[];
extern const uintptr_t hierarchy_UICombinedCardItemCollection_ComparisonWnd[];
extern const uintptr_t bases_UICombinedCardItemCollection_ComparisonWnd[];
extern const uintptr_t base_UICombinedCardItemCollection_ComparisonWnd[];
extern const uintptr_t locator_UISkillDescribeWnd[];
extern const uintptr_t hierarchy_UISkillDescribeWnd[];
extern const uintptr_t bases_UISkillDescribeWnd[];
extern const uintptr_t base_UISkillDescribeWnd[];
extern const uintptr_t locator_UICardItemIllustWnd[];
extern const uintptr_t hierarchy_UICardItemIllustWnd[];
extern const uintptr_t bases_UICardItemIllustWnd[];
extern const uintptr_t base_UICardItemIllustWnd[];
extern const uintptr_t locator_UIMonsterDisplayWnd[];
extern const uintptr_t hierarchy_UIMonsterDisplayWnd[];
extern const uintptr_t bases_UIMonsterDisplayWnd[];
extern const uintptr_t base_UIMonsterDisplayWnd[];
extern const uintptr_t locator_UISayDialogWnd[];
extern const uintptr_t hierarchy_UISayDialogWnd[];
extern const uintptr_t bases_UISayDialogWnd[];
extern const uintptr_t base_UISayDialogWnd[];
extern const uintptr_t locator_UIChooseWnd[];
extern const uintptr_t hierarchy_UIChooseWnd[];
extern const uintptr_t bases_UIChooseWnd[];
extern const uintptr_t base_UIChooseWnd[];
extern const uintptr_t locator_UIChooseWarpWnd[];
extern const uintptr_t hierarchy_UIChooseWarpWnd[];
extern const uintptr_t bases_UIChooseWarpWnd[];
extern const uintptr_t base_UIChooseWarpWnd[];
extern const uintptr_t locator_UIRestartWnd[];
extern const uintptr_t hierarchy_UIRestartWnd[];
extern const uintptr_t bases_UIRestartWnd[];
extern const uintptr_t base_UIRestartWnd[];
extern const uintptr_t locator_UIQuitWnd[];
extern const uintptr_t hierarchy_UIQuitWnd[];
extern const uintptr_t bases_UIQuitWnd[];
extern const uintptr_t base_UIQuitWnd[];
extern const uintptr_t locator_UINoticeConfirmWnd[];
extern const uintptr_t hierarchy_UINoticeConfirmWnd[];
extern const uintptr_t bases_UINoticeConfirmWnd[];
extern const uintptr_t base_UINoticeConfirmWnd[];
extern const uintptr_t locator_UIChooseSellBuyWnd[];
extern const uintptr_t hierarchy_UIChooseSellBuyWnd[];
extern const uintptr_t bases_UIChooseSellBuyWnd[];
extern const uintptr_t base_UIChooseSellBuyWnd[];
extern const uintptr_t locator_UIDisconnectedServerMsgWnd[];
extern const uintptr_t hierarchy_UIDisconnectedServerMsgWnd[];
extern const uintptr_t bases_UIDisconnectedServerMsgWnd[];
extern const uintptr_t base_UIDisconnectedServerMsgWnd[];
extern const uintptr_t locator_UIExchangeAcceptWnd[];
extern const uintptr_t hierarchy_UIExchangeAcceptWnd[];
extern const uintptr_t bases_UIExchangeAcceptWnd[];
extern const uintptr_t base_UIExchangeAcceptWnd[];
extern const uintptr_t locator_UIJoinPartyAcceptWnd[];
extern const uintptr_t hierarchy_UIJoinPartyAcceptWnd[];
extern const uintptr_t bases_UIJoinPartyAcceptWnd[];
extern const uintptr_t base_UIJoinPartyAcceptWnd[];
extern const uintptr_t locator_UICoupleAcceptWnd[];
extern const uintptr_t hierarchy_UICoupleAcceptWnd[];
extern const uintptr_t bases_UICoupleAcceptWnd[];
extern const uintptr_t base_UICoupleAcceptWnd[];
extern const uintptr_t locator_UIStarPlaceAcceptWnd[];
extern const uintptr_t hierarchy_UIStarPlaceAcceptWnd[];
extern const uintptr_t bases_UIStarPlaceAcceptWnd[];
extern const uintptr_t base_UIStarPlaceAcceptWnd[];
extern const uintptr_t locator_UIMixAcceptWnd[];
extern const uintptr_t hierarchy_UIMixAcceptWnd[];
extern const uintptr_t bases_UIMixAcceptWnd[];
extern const uintptr_t base_UIMixAcceptWnd[];
extern const uintptr_t locator_UIYourItemWnd[];
extern const uintptr_t hierarchy_UIYourItemWnd[];
extern const uintptr_t bases_UIYourItemWnd[];
extern const uintptr_t base_UIYourItemWnd[];
extern const uintptr_t locator_UIBabyAcceptWnd[];
extern const uintptr_t hierarchy_UIBabyAcceptWnd[];
extern const uintptr_t bases_UIBabyAcceptWnd[];
extern const uintptr_t base_UIBabyAcceptWnd[];
extern const uintptr_t locator_UIBabyAcceptWnd2[];
extern const uintptr_t hierarchy_UIBabyAcceptWnd2[];
extern const uintptr_t bases_UIBabyAcceptWnd2[];
extern const uintptr_t base_UIBabyAcceptWnd2[];
extern const uintptr_t locator_UIJoinGuildAcceptWnd[];
extern const uintptr_t hierarchy_UIJoinGuildAcceptWnd[];
extern const uintptr_t bases_UIJoinGuildAcceptWnd[];
extern const uintptr_t base_UIJoinGuildAcceptWnd[];
extern const uintptr_t locator_UITipOfTheDayWnd[];
extern const uintptr_t hierarchy_UITipOfTheDayWnd[];
extern const uintptr_t bases_UITipOfTheDayWnd[];
extern const uintptr_t base_UITipOfTheDayWnd[];
extern const uintptr_t locator_UIChatRoomMakeWnd[];
extern const uintptr_t hierarchy_UIChatRoomMakeWnd[];
extern const uintptr_t bases_UIChatRoomMakeWnd[];
extern const uintptr_t base_UIChatRoomMakeWnd[];
extern const uintptr_t locator_UIChatRoomChangeWnd[];
extern const uintptr_t hierarchy_UIChatRoomChangeWnd[];
extern const uintptr_t bases_UIChatRoomChangeWnd[];
extern const uintptr_t base_UIChatRoomChangeWnd[];
extern const uintptr_t locator_UIPartySettingWnd[];
extern const uintptr_t hierarchy_UIPartySettingWnd[];
extern const uintptr_t bases_UIPartySettingWnd[];
extern const uintptr_t base_UIPartySettingWnd[];
extern const uintptr_t locator_UIPartyInvitationToWnd[];
extern const uintptr_t hierarchy_UIPartyInvitationToWnd[];
extern const uintptr_t bases_UIPartyInvitationToWnd[];
extern const uintptr_t base_UIPartyInvitationToWnd[];
extern const uintptr_t locator_UIWhisperWnd[];
extern const uintptr_t hierarchy_UIWhisperWnd[];
extern const uintptr_t bases_UIWhisperWnd[];
extern const uintptr_t base_UIWhisperWnd[];
extern const uintptr_t locator_UIItemIdentifyWnd[];
extern const uintptr_t hierarchy_UIItemIdentifyWnd[];
extern const uintptr_t bases_UIItemIdentifyWnd[];
extern const uintptr_t base_UIItemIdentifyWnd[];
extern const uintptr_t locator_UIPetEggListWnd[];
extern const uintptr_t hierarchy_UIPetEggListWnd[];
extern const uintptr_t bases_UIPetEggListWnd[];
extern const uintptr_t base_UIPetEggListWnd[];
extern const uintptr_t locator_UIMakingArrowListWnd[];
extern const uintptr_t hierarchy_UIMakingArrowListWnd[];
extern const uintptr_t bases_UIMakingArrowListWnd[];
extern const uintptr_t base_UIMakingArrowListWnd[];
extern const uintptr_t locator_UIItemCompositionWnd[];
extern const uintptr_t hierarchy_UIItemCompositionWnd[];
extern const uintptr_t bases_UIItemCompositionWnd[];
extern const uintptr_t base_UIItemCompositionWnd[];
extern const uintptr_t locator_UIMenuWnd[];
extern const uintptr_t hierarchy_UIMenuWnd[];
extern const uintptr_t bases_UIMenuWnd[];
extern const uintptr_t base_UIMenuWnd[];
extern const uintptr_t locator_UIComboBoxWnd[];
extern const uintptr_t hierarchy_UIComboBoxWnd[];
extern const uintptr_t bases_UIComboBoxWnd[];
extern const uintptr_t base_UIComboBoxWnd[];
extern const uintptr_t locator_UIComboBoxIDWnd[];
extern const uintptr_t hierarchy_UIComboBoxIDWnd[];
extern const uintptr_t bases_UIComboBoxIDWnd[];
extern const uintptr_t base_UIComboBoxIDWnd[];
extern const uintptr_t locator_UIPetTamingDeceiveWnd[];
extern const uintptr_t hierarchy_UIPetTamingDeceiveWnd[];
extern const uintptr_t bases_UIPetTamingDeceiveWnd[];
extern const uintptr_t base_UIPetTamingDeceiveWnd[];
extern const uintptr_t locator_UISelectCartWnd[];
extern const uintptr_t hierarchy_UISelectCartWnd[];
extern const uintptr_t bases_UISelectCartWnd[];
extern const uintptr_t base_UISelectCartWnd[];
extern const uintptr_t locator_UISpellListWnd[];
extern const uintptr_t hierarchy_UISpellListWnd[];
extern const uintptr_t bases_UISpellListWnd[];
extern const uintptr_t base_UISpellListWnd[];
extern const uintptr_t locator_UIGraffiStrboxWnd[];
extern const uintptr_t hierarchy_UIGraffiStrboxWnd[];
extern const uintptr_t bases_UIGraffiStrboxWnd[];
extern const uintptr_t base_UIGraffiStrboxWnd[];
extern const uintptr_t locator_UIProhibitListWnd[];
extern const uintptr_t hierarchy_UIProhibitListWnd[];
extern const uintptr_t bases_UIProhibitListWnd[];
extern const uintptr_t base_UIProhibitListWnd[];
extern const uintptr_t locator_UIBookWnd[];
extern const uintptr_t hierarchy_UIBookWnd[];
extern const uintptr_t bases_UIBookWnd[];
extern const uintptr_t base_UIBookWnd[];
extern const uintptr_t locator_UIJoinFriendAcceptWnd[];
extern const uintptr_t hierarchy_UIJoinFriendAcceptWnd[];
extern const uintptr_t bases_UIJoinFriendAcceptWnd[];
extern const uintptr_t base_UIJoinFriendAcceptWnd[];
extern const uintptr_t locator_UICommandListWnd[];
extern const uintptr_t hierarchy_UICommandListWnd[];
extern const uintptr_t bases_UICommandListWnd[];
extern const uintptr_t base_UICommandListWnd[];
extern const uintptr_t locator_UIStorePasswordInputWnd[];
extern const uintptr_t hierarchy_UIStorePasswordInputWnd[];
extern const uintptr_t bases_UIStorePasswordInputWnd[];
extern const uintptr_t base_UIStorePasswordInputWnd[];
extern const uintptr_t locator_UISecondPasswordInputWnd[];
extern const uintptr_t hierarchy_UISecondPasswordInputWnd[];
extern const uintptr_t bases_UISecondPasswordInputWnd[];
extern const uintptr_t base_UISecondPasswordInputWnd[];
extern const uintptr_t locator_UIAutoMessageWnd[];
extern const uintptr_t hierarchy_UIAutoMessageWnd[];
extern const uintptr_t bases_UIAutoMessageWnd[];
extern const uintptr_t base_UIAutoMessageWnd[];
extern const uintptr_t locator_UITipboxWnd[];
extern const uintptr_t hierarchy_UITipboxWnd[];
extern const uintptr_t bases_UITipboxWnd[];
extern const uintptr_t base_UITipboxWnd[];
extern const uintptr_t locator_UIShortCutWnd[];
extern const uintptr_t hierarchy_UIShortCutWnd[];
extern const uintptr_t bases_UIShortCutWnd[];
extern const uintptr_t base_UIShortCutWnd[];
extern const uintptr_t locator_UICharInfoBalloonText[];
extern const uintptr_t hierarchy_UICharInfoBalloonText[];
extern const uintptr_t bases_UICharInfoBalloonText[];
extern const uintptr_t base_UICharInfoBalloonText[];
extern const uintptr_t locator_UISelCharForUServerWnd[];
extern const uintptr_t hierarchy_UISelCharForUServerWnd[];
extern const uintptr_t bases_UISelCharForUServerWnd[];
extern const uintptr_t base_UISelCharForUServerWnd[];
extern const uintptr_t locator_UIPartyBookingHelpWnd[];
extern const uintptr_t hierarchy_UIPartyBookingHelpWnd[];
extern const uintptr_t bases_UIPartyBookingHelpWnd[];
extern const uintptr_t base_UIPartyBookingHelpWnd[];
extern const uintptr_t locator_UISeekPartyWnd[];
extern const uintptr_t hierarchy_UISeekPartyWnd[];
extern const uintptr_t bases_UISeekPartyWnd[];
extern const uintptr_t base_UISeekPartyWnd[];
extern const uintptr_t locator_UIJobListWnd[];
extern const uintptr_t hierarchy_UIJobListWnd[];
extern const uintptr_t bases_UIJobListWnd[];
extern const uintptr_t base_UIJobListWnd[];
extern const uintptr_t locator_UISeekPartyMBWnd[];
extern const uintptr_t hierarchy_UISeekPartyMBWnd[];
extern const uintptr_t bases_UISeekPartyMBWnd[];
extern const uintptr_t base_UISeekPartyMBWnd[];
extern const uintptr_t locator_UISeekPartyListWnd[];
extern const uintptr_t hierarchy_UISeekPartyListWnd[];
extern const uintptr_t bases_UISeekPartyListWnd[];
extern const uintptr_t base_UISeekPartyListWnd[];
extern const uintptr_t locator_UIChangeNameWnd[];
extern const uintptr_t hierarchy_UIChangeNameWnd[];
extern const uintptr_t bases_UIChangeNameWnd[];
extern const uintptr_t base_UIChangeNameWnd[];
extern const uintptr_t locator_UIRebirthWnd[];
extern const uintptr_t hierarchy_UIRebirthWnd[];
extern const uintptr_t bases_UIRebirthWnd[];
extern const uintptr_t base_UIRebirthWnd[];
extern const uintptr_t locator_UISubChatMiniWnd[];
extern const uintptr_t hierarchy_UISubChatMiniWnd[];
extern const uintptr_t bases_UISubChatMiniWnd[];
extern const uintptr_t base_UISubChatMiniWnd[];
extern const uintptr_t locator_UIBattleMsgOptionWnd[];
extern const uintptr_t hierarchy_UIBattleMsgOptionWnd[];
extern const uintptr_t bases_UIBattleMsgOptionWnd[];
extern const uintptr_t base_UIBattleMsgOptionWnd[];
extern const uintptr_t locator_UISubChatHisWnd[];
extern const uintptr_t hierarchy_UISubChatHisWnd[];
extern const uintptr_t bases_UISubChatHisWnd[];
extern const uintptr_t base_UISubChatHisWnd[];
extern const uintptr_t locator_UINewChatWnd[];
extern const uintptr_t hierarchy_UINewChatWnd[];
extern const uintptr_t bases_UINewChatWnd[];
extern const uintptr_t base_UINewChatWnd[];
extern const uintptr_t locator_UIMemorialDunWnd[];
extern const uintptr_t hierarchy_UIMemorialDunWnd[];
extern const uintptr_t bases_UIMemorialDunWnd[];
extern const uintptr_t base_UIMemorialDunWnd[];
extern const uintptr_t locator_UIRoMapWnd[];
extern const uintptr_t hierarchy_UIRoMapWnd[];
extern const uintptr_t bases_UIRoMapWnd[];
extern const uintptr_t base_UIRoMapWnd[];
extern const uintptr_t locator_UIMiniBtnWnd[];
extern const uintptr_t hierarchy_UIMiniBtnWnd[];
extern const uintptr_t bases_UIMiniBtnWnd[];
extern const uintptr_t base_UIMiniBtnWnd[];
extern const uintptr_t locator_UIHotKeyWnd[];
extern const uintptr_t hierarchy_UIHotKeyWnd[];
extern const uintptr_t bases_UIHotKeyWnd[];
extern const uintptr_t base_UIHotKeyWnd[];
extern const uintptr_t locator_UIEscOptionWnd[];
extern const uintptr_t hierarchy_UIEscOptionWnd[];
extern const uintptr_t bases_UIEscOptionWnd[];
extern const uintptr_t base_UIEscOptionWnd[];
extern const uintptr_t locator_UIQuikSlotWnd[];
extern const uintptr_t hierarchy_UIQuikSlotWnd[];
extern const uintptr_t bases_UIQuikSlotWnd[];
extern const uintptr_t base_UIQuikSlotWnd[];
extern const uintptr_t locator_UISerachStoreInfoWnd[];
extern const uintptr_t hierarchy_UISerachStoreInfoWnd[];
extern const uintptr_t bases_UISerachStoreInfoWnd[];
extern const uintptr_t base_UISerachStoreInfoWnd[];
extern const uintptr_t locator_UIButtonsOfBasicWnd[];
extern const uintptr_t hierarchy_UIButtonsOfBasicWnd[];
extern const uintptr_t bases_UIButtonsOfBasicWnd[];
extern const uintptr_t base_UIButtonsOfBasicWnd[];
extern const uintptr_t locator_UINavigationWnd[];
extern const uintptr_t hierarchy_UINavigationWnd[];
extern const uintptr_t bases_UINavigationWnd[];
extern const uintptr_t base_UINavigationWnd[];
extern const uintptr_t locator_UINavigationHelpWnd[];
extern const uintptr_t hierarchy_UINavigationHelpWnd[];
extern const uintptr_t bases_UINavigationHelpWnd[];
extern const uintptr_t base_UINavigationHelpWnd[];
extern const uintptr_t locator_CUICollectionSystemWnd[];
extern const uintptr_t hierarchy_CUICollectionSystemWnd[];
extern const uintptr_t bases_CUICollectionSystemWnd[];
extern const uintptr_t base_CUICollectionSystemWnd[];
extern const uintptr_t locator_UIGuildInfoManageWnd[];
extern const uintptr_t hierarchy_UIGuildInfoManageWnd[];
extern const uintptr_t bases_UIGuildInfoManageWnd[];
extern const uintptr_t base_UIGuildInfoManageWnd[];
extern const uintptr_t base_UIGuildWnd[];
extern const uintptr_t hierarchy_UIGuildWnd[];
extern const uintptr_t bases_UIGuildWnd[];
extern const uintptr_t locator_UIGuildMemberManageWnd[];
extern const uintptr_t hierarchy_UIGuildMemberManageWnd[];
extern const uintptr_t bases_UIGuildMemberManageWnd[];
extern const uintptr_t base_UIGuildMemberManageWnd[];
extern const uintptr_t locator_UIGuildPositionManageWnd[];
extern const uintptr_t hierarchy_UIGuildPositionManageWnd[];
extern const uintptr_t bases_UIGuildPositionManageWnd[];
extern const uintptr_t base_UIGuildPositionManageWnd[];
extern const uintptr_t locator_UIGuildSkillWnd[];
extern const uintptr_t hierarchy_UIGuildSkillWnd[];
extern const uintptr_t bases_UIGuildSkillWnd[];
extern const uintptr_t base_UIGuildSkillWnd[];
extern const uintptr_t locator_UIGuildBanishedMemberWnd[];
extern const uintptr_t hierarchy_UIGuildBanishedMemberWnd[];
extern const uintptr_t bases_UIGuildBanishedMemberWnd[];
extern const uintptr_t base_UIGuildBanishedMemberWnd[];
extern const uintptr_t locator_UIGuildNoticeWnd[];
extern const uintptr_t hierarchy_UIGuildNoticeWnd[];
extern const uintptr_t bases_UIGuildNoticeWnd[];
extern const uintptr_t base_UIGuildNoticeWnd[];
extern const uintptr_t locator_UIGuildTotalInfoWnd[];
extern const uintptr_t hierarchy_UIGuildTotalInfoWnd[];
extern const uintptr_t bases_UIGuildTotalInfoWnd[];
extern const uintptr_t base_UIGuildTotalInfoWnd[];
extern const uintptr_t locator_UIGuildLeaveReasonDescWnd[];
extern const uintptr_t hierarchy_UIGuildLeaveReasonDescWnd[];
extern const uintptr_t bases_UIGuildLeaveReasonDescWnd[];
extern const uintptr_t base_UIGuildLeaveReasonDescWnd[];
extern const uintptr_t locator_UIGuildBanReasonDescWnd[];
extern const uintptr_t hierarchy_UIGuildBanReasonDescWnd[];
extern const uintptr_t bases_UIGuildBanReasonDescWnd[];
extern const uintptr_t base_UIGuildBanReasonDescWnd[];
extern const uintptr_t locator_UIGuildHelperWnd[];
extern const uintptr_t hierarchy_UIGuildHelperWnd[];
extern const uintptr_t bases_UIGuildHelperWnd[];
extern const uintptr_t base_UIGuildHelperWnd[];
extern const uintptr_t locator_UIGuildTipWnd[];
extern const uintptr_t hierarchy_UIGuildTipWnd[];
extern const uintptr_t bases_UIGuildTipWnd[];
extern const uintptr_t base_UIGuildTipWnd[];
extern const uintptr_t locator_UICreateGuildWnd[];
extern const uintptr_t hierarchy_UICreateGuildWnd[];
extern const uintptr_t bases_UICreateGuildWnd[];
extern const uintptr_t base_UICreateGuildWnd[];
extern const uintptr_t locator_UIAgitMessageBox[];
extern const uintptr_t hierarchy_UIAgitMessageBox[];
extern const uintptr_t bases_UIAgitMessageBox[];
extern const uintptr_t base_UIAgitMessageBox[];
extern const uintptr_t locator_UIListBoxAgitMenu[];
extern const uintptr_t hierarchy_UIListBoxAgitMenu[];
extern const uintptr_t bases_UIListBoxAgitMenu[];
extern const uintptr_t base_UIListBoxAgitMenu[];
extern const uintptr_t locator_UIListBoxAgit[];
extern const uintptr_t hierarchy_UIListBoxAgit[];
extern const uintptr_t bases_UIListBoxAgit[];
extern const uintptr_t base_UIListBoxAgit[];
extern const uintptr_t locator_UIGuildWnd[];
extern const uintptr_t locator_UIEditCtrlWheel[];
extern const uintptr_t hierarchy_UIEditCtrlWheel[];
extern const uintptr_t bases_UIEditCtrlWheel[];
extern const uintptr_t base_UIEditCtrlWheel[];
extern const uintptr_t locator_UIItemBaseWnd[];
extern const uintptr_t locator_UIItemStoreSubWnd[];
extern const uintptr_t hierarchy_UIItemStoreSubWnd[];
extern const uintptr_t bases_UIItemStoreSubWnd[];
extern const uintptr_t base_UIItemStoreSubWnd[];
extern const uintptr_t locator_UIItemStoreFindWnd[];
extern const uintptr_t hierarchy_UIItemStoreFindWnd[];
extern const uintptr_t bases_UIItemStoreFindWnd[];
extern const uintptr_t base_UIItemStoreFindWnd[];
extern const uintptr_t locator_UIItemStoreWnd[];
extern const uintptr_t hierarchy_UIItemStoreWnd[];
extern const uintptr_t bases_UIItemStoreWnd[];
extern const uintptr_t base_UIItemStoreWnd[];
extern const uintptr_t locator_UISkillListWnd[];
extern const uintptr_t hierarchy_UISkillListWnd[];
extern const uintptr_t bases_UISkillListWnd[];
extern const uintptr_t base_UISkillListWnd[];
extern const uintptr_t locator_UIItemShopWnd[];
extern const uintptr_t hierarchy_UIItemShopWnd[];
extern const uintptr_t bases_UIItemShopWnd[];
extern const uintptr_t base_UIItemShopWnd[];
extern const uintptr_t locator_UIItemShopWnd2[];
extern const uintptr_t hierarchy_UIItemShopWnd2[];
extern const uintptr_t bases_UIItemShopWnd2[];
extern const uintptr_t base_UIItemShopWnd2[];
extern const uintptr_t locator_UIItemPurchaseWnd[];
extern const uintptr_t hierarchy_UIItemPurchaseWnd[];
extern const uintptr_t bases_UIItemPurchaseWnd[];
extern const uintptr_t base_UIItemPurchaseWnd[];
extern const uintptr_t locator_UIItemSellWnd[];
extern const uintptr_t hierarchy_UIItemSellWnd[];
extern const uintptr_t bases_UIItemSellWnd[];
extern const uintptr_t base_UIItemSellWnd[];
extern const uintptr_t locator_UIItemSellWnd2[];
extern const uintptr_t hierarchy_UIItemSellWnd2[];
extern const uintptr_t bases_UIItemSellWnd2[];
extern const uintptr_t base_UIItemSellWnd2[];
extern const uintptr_t locator_UIMerchantItemShopWnd[];
extern const uintptr_t hierarchy_UIMerchantItemShopWnd[];
extern const uintptr_t bases_UIMerchantItemShopWnd[];
extern const uintptr_t base_UIMerchantItemShopWnd[];
extern const uintptr_t locator_UIMerchantItemMyShopWnd[];
extern const uintptr_t hierarchy_UIMerchantItemMyShopWnd[];
extern const uintptr_t bases_UIMerchantItemMyShopWnd[];
extern const uintptr_t base_UIMerchantItemMyShopWnd[];
extern const uintptr_t locator_UIMerchantItemPurchaseMSGWnd[];
extern const uintptr_t hierarchy_UIMerchantItemPurchaseMSGWnd[];
extern const uintptr_t bases_UIMerchantItemPurchaseMSGWnd[];
extern const uintptr_t base_UIMerchantItemPurchaseMSGWnd[];
extern const uintptr_t locator_UIMerchantItemPurchaseWnd[];
extern const uintptr_t hierarchy_UIMerchantItemPurchaseWnd[];
extern const uintptr_t bases_UIMerchantItemPurchaseWnd[];
extern const uintptr_t base_UIMerchantItemPurchaseWnd[];
extern const uintptr_t locator_UINoticeMessageBox_LockSellItem[];
extern const uintptr_t hierarchy_UINoticeMessageBox_LockSellItem[];
extern const uintptr_t bases_UINoticeMessageBox_LockSellItem[];
extern const uintptr_t base_UINoticeMessageBox_LockSellItem[];
extern const uintptr_t locator_UIItemWnd[];
extern const uintptr_t hierarchy_UIItemWnd[];
extern const uintptr_t bases_UIItemWnd[];
extern const uintptr_t base_UIItemWnd[];
extern const uintptr_t locator_UIMerchantItemWnd[];
extern const uintptr_t hierarchy_UIMerchantItemWnd[];
extern const uintptr_t bases_UIMerchantItemWnd[];
extern const uintptr_t base_UIMerchantItemWnd[];
extern const uintptr_t locator_UIMerchantMirrorItemWnd[];
extern const uintptr_t hierarchy_UIMerchantMirrorItemWnd[];
extern const uintptr_t bases_UIMerchantMirrorItemWnd[];
extern const uintptr_t base_UIMerchantMirrorItemWnd[];
extern const uintptr_t locator_UIItemDropCntWnd[];
extern const uintptr_t locator_UIMerchantShopMakeWnd[];
extern const uintptr_t hierarchy_UIMerchantShopMakeWnd[];
extern const uintptr_t bases_UIMerchantShopMakeWnd[];
extern const uintptr_t base_UIMerchantShopMakeWnd[];
extern const uintptr_t locator_UIMinimapZoomWnd[];
extern const uintptr_t hierarchy_UIMinimapZoomWnd[];
extern const uintptr_t bases_UIMinimapZoomWnd[];
extern const uintptr_t base_UIMinimapZoomWnd[];
extern const uintptr_t locator_UINotifyLevelUpWnd[];
extern const uintptr_t hierarchy_UINotifyLevelUpWnd[];
extern const uintptr_t bases_UINotifyLevelUpWnd[];
extern const uintptr_t base_UINotifyLevelUpWnd[];
extern const uintptr_t locator_UINotifyJobLevelUpWnd[];
extern const uintptr_t hierarchy_UINotifyJobLevelUpWnd[];
extern const uintptr_t bases_UINotifyJobLevelUpWnd[];
extern const uintptr_t base_UINotifyJobLevelUpWnd[];
extern const uintptr_t locator_UINotifyQuestWnd[];
extern const uintptr_t hierarchy_UINotifyQuestWnd[];
extern const uintptr_t bases_UINotifyQuestWnd[];
extern const uintptr_t base_UINotifyQuestWnd[];
extern const uintptr_t locator_UInCash_CallWnd[];
extern const uintptr_t hierarchy_UInCash_CallWnd[];
extern const uintptr_t bases_UInCash_CallWnd[];
extern const uintptr_t base_UInCash_CallWnd[];
extern const uintptr_t locator_UISecondPasswordEditWnd[];
extern const uintptr_t hierarchy_UISecondPasswordEditWnd[];
extern const uintptr_t bases_UISecondPasswordEditWnd[];
extern const uintptr_t base_UISecondPasswordEditWnd[];
extern const uintptr_t locator_UIEmotionWnd[];
extern const uintptr_t hierarchy_UIEmotionWnd[];
extern const uintptr_t bases_UIEmotionWnd[];
extern const uintptr_t base_UIEmotionWnd[];
extern const uintptr_t locator_CEffectPrim[];
extern const uintptr_t hierarchy_CEffectPrim[];
extern const uintptr_t bases_CEffectPrim[];
extern const uintptr_t base_CEffectPrim[];
extern const uintptr_t locator_CRagEffect[];
extern const uintptr_t locator_CMousePointer[];
extern const uintptr_t hierarchy_CMousePointer[];
extern const uintptr_t bases_CMousePointer[];
extern const uintptr_t base_CMousePointer[];
extern const uintptr_t locator_CGameActor[];
extern const uintptr_t locator_CRenderObject[];
extern const uintptr_t locator_CAbleToMakeEffect[];
extern const uintptr_t locator_C3dGrannyGameActor[];
extern const uintptr_t hierarchy_C3dGrannyGameActor[];
extern const uintptr_t bases_C3dGrannyGameActor[];
extern const uintptr_t base_C3dGrannyGameActor[];
extern const uintptr_t locator_UIVerticalNameBalloonText[];
extern const uintptr_t hierarchy_UIVerticalNameBalloonText[];
extern const uintptr_t bases_UIVerticalNameBalloonText[];
extern const uintptr_t base_UIVerticalNameBalloonText[];
extern const uintptr_t locator_CDummyEffectLauncher[];
extern const uintptr_t hierarchy_CDummyEffectLauncher[];
extern const uintptr_t bases_CDummyEffectLauncher[];
extern const uintptr_t base_CDummyEffectLauncher[];
extern const uintptr_t locator_CItem[];
extern const uintptr_t hierarchy_CItem[];
extern const uintptr_t bases_CItem[];
extern const uintptr_t base_CItem[];
extern const uintptr_t locator_CCorpse[];
extern const uintptr_t hierarchy_CCorpse[];
extern const uintptr_t bases_CCorpse[];
extern const uintptr_t base_CCorpse[];
extern const uintptr_t locator_CGndRiser[];
extern const uintptr_t hierarchy_CGndRiser[];
extern const uintptr_t bases_CGndRiser[];
extern const uintptr_t base_CGndRiser[];
extern const uintptr_t locator_CSoundMaker[];
extern const uintptr_t hierarchy_CSoundMaker[];
extern const uintptr_t bases_CSoundMaker[];
extern const uintptr_t base_CSoundMaker[];
extern const uintptr_t locator_CNpc[];
extern const uintptr_t locator_CBlowEffect[];
extern const uintptr_t hierarchy_CBlowEffect[];
extern const uintptr_t bases_CBlowEffect[];
extern const uintptr_t base_CBlowEffect[];
extern const uintptr_t locator_CBlurPC[];
extern const uintptr_t hierarchy_CBlurPC[];
extern const uintptr_t bases_CBlurPC[];
extern const uintptr_t base_CBlurPC[];
extern const uintptr_t base_CPc[];
extern const uintptr_t hierarchy_CPc[];
extern const uintptr_t bases_CPc[];
extern const uintptr_t locator_CHalluPC[];
extern const uintptr_t hierarchy_CHalluPC[];
extern const uintptr_t bases_CHalluPC[];
extern const uintptr_t base_CHalluPC[];
extern const uintptr_t locator_CHalluPC2[];
extern const uintptr_t hierarchy_CHalluPC2[];
extern const uintptr_t bases_CHalluPC2[];
extern const uintptr_t base_CHalluPC2[];
extern const uintptr_t locator_CHalluNPC[];
extern const uintptr_t hierarchy_CHalluNPC[];
extern const uintptr_t bases_CHalluNPC[];
extern const uintptr_t base_CHalluNPC[];
extern const uintptr_t locator_CMasterChaser[];
extern const uintptr_t hierarchy_CMasterChaser[];
extern const uintptr_t bases_CMasterChaser[];
extern const uintptr_t base_CMasterChaser[];
extern const uintptr_t locator_CNumEffect[];
extern const uintptr_t hierarchy_CNumEffect[];
extern const uintptr_t bases_CNumEffect[];
extern const uintptr_t base_CNumEffect[];
extern const uintptr_t locator_CHalluPlant[];
extern const uintptr_t hierarchy_CHalluPlant[];
extern const uintptr_t bases_CHalluPlant[];
extern const uintptr_t base_CHalluPlant[];
extern const uintptr_t locator_CPc[];
extern const uintptr_t locator_CGrannyPc[];
extern const uintptr_t hierarchy_CGrannyPc[];
extern const uintptr_t bases_CGrannyPc[];
extern const uintptr_t base_CGrannyPc[];
extern const uintptr_t locator_CPlayer[];
extern const uintptr_t hierarchy_CPlayer[];
extern const uintptr_t bases_CPlayer[];
extern const uintptr_t base_CPlayer[];
extern const uintptr_t locator_CMsgEffect[];
extern const uintptr_t hierarchy_CMsgEffect[];
extern const uintptr_t bases_CMsgEffect[];
extern const uintptr_t base_CMsgEffect[];
extern const uintptr_t locator_CGroundScope[];
extern const uintptr_t hierarchy_CGroundScope[];
extern const uintptr_t bases_CGroundScope[];
extern const uintptr_t base_CGroundScope[];
extern const uintptr_t locator_CEffectMaker[];
extern const uintptr_t hierarchy_CEffectMaker[];
extern const uintptr_t bases_CEffectMaker[];
extern const uintptr_t base_CEffectMaker[];
extern const uintptr_t locator_CSkill[];
extern const uintptr_t hierarchy_CSkill[];
extern const uintptr_t bases_CSkill[];
extern const uintptr_t base_CSkill[];
extern const uintptr_t locator_CGraffiSkill[];
extern const uintptr_t hierarchy_CGraffiSkill[];
extern const uintptr_t bases_CGraffiSkill[];
extern const uintptr_t base_CGraffiSkill[];
extern const uintptr_t locator_CArrowEffect[];
extern const uintptr_t hierarchy_CArrowEffect[];
extern const uintptr_t bases_CArrowEffect[];
extern const uintptr_t base_CArrowEffect[];
extern const uintptr_t locator_CMandraAttackEffect[];
extern const uintptr_t hierarchy_CMandraAttackEffect[];
extern const uintptr_t bases_CMandraAttackEffect[];
extern const uintptr_t base_CMandraAttackEffect[];
extern const uintptr_t locator_UIEnchant_Tab[];
extern const uintptr_t hierarchy_UIEnchant_Tab[];
extern const uintptr_t bases_UIEnchant_Tab[];
extern const uintptr_t base_UIEnchant_Tab[];
extern const uintptr_t locator_UIEnchant_Group[];
extern const uintptr_t hierarchy_UIEnchant_Group[];
extern const uintptr_t bases_UIEnchant_Group[];
extern const uintptr_t base_UIEnchant_Group[];
extern const uintptr_t locator_UIEnchant_ScrollBar[];
extern const uintptr_t hierarchy_UIEnchant_ScrollBar[];
extern const uintptr_t bases_UIEnchant_ScrollBar[];
extern const uintptr_t base_UIEnchant_ScrollBar[];
extern const uintptr_t locator_UIEnchant_List[];
extern const uintptr_t hierarchy_UIEnchant_List[];
extern const uintptr_t bases_UIEnchant_List[];
extern const uintptr_t base_UIEnchant_List[];
extern const uintptr_t locator_UIEnchant_TargetList[];
extern const uintptr_t hierarchy_UIEnchant_TargetList[];
extern const uintptr_t bases_UIEnchant_TargetList[];
extern const uintptr_t base_UIEnchant_TargetList[];
extern const uintptr_t locator_UIEnchant_Item[];
extern const uintptr_t hierarchy_UIEnchant_Item[];
extern const uintptr_t bases_UIEnchant_Item[];
extern const uintptr_t base_UIEnchant_Item[];
extern const uintptr_t locator_UIEnchant_Button[];
extern const uintptr_t hierarchy_UIEnchant_Button[];
extern const uintptr_t bases_UIEnchant_Button[];
extern const uintptr_t base_UIEnchant_Button[];
extern const uintptr_t locator_UIEnchant_Info[];
extern const uintptr_t hierarchy_UIEnchant_Info[];
extern const uintptr_t bases_UIEnchant_Info[];
extern const uintptr_t base_UIEnchant_Info[];
extern const uintptr_t locator_UIEnchantWnd[];
extern const uintptr_t hierarchy_UIEnchantWnd[];
extern const uintptr_t bases_UIEnchantWnd[];
extern const uintptr_t base_UIEnchantWnd[];
extern const uintptr_t locator_UIItemReformWnd_List[];
extern const uintptr_t hierarchy_UIItemReformWnd_List[];
extern const uintptr_t bases_UIItemReformWnd_List[];
extern const uintptr_t base_UIItemReformWnd_List[];
extern const uintptr_t locator_UIItemReformWnd_Item[];
extern const uintptr_t hierarchy_UIItemReformWnd_Item[];
extern const uintptr_t bases_UIItemReformWnd_Item[];
extern const uintptr_t base_UIItemReformWnd_Item[];
extern const uintptr_t locator_UIItemReformWnd_DetailBtn[];
extern const uintptr_t hierarchy_UIItemReformWnd_DetailBtn[];
extern const uintptr_t bases_UIItemReformWnd_DetailBtn[];
extern const uintptr_t base_UIItemReformWnd_DetailBtn[];
extern const uintptr_t locator_UIItemReformWnd_DetailMsg[];
extern const uintptr_t hierarchy_UIItemReformWnd_DetailMsg[];
extern const uintptr_t bases_UIItemReformWnd_DetailMsg[];
extern const uintptr_t base_UIItemReformWnd_DetailMsg[];
extern const uintptr_t locator_UIItemReformWnd[];
extern const uintptr_t hierarchy_UIItemReformWnd[];
extern const uintptr_t bases_UIItemReformWnd[];
extern const uintptr_t base_UIItemReformWnd[];
extern const uintptr_t locator_UILapineScrollBar[];
extern const uintptr_t hierarchy_UILapineScrollBar[];
extern const uintptr_t bases_UILapineScrollBar[];
extern const uintptr_t base_UILapineScrollBar[];
extern const uintptr_t locator_UIIngredientInfoWnd[];
extern const uintptr_t hierarchy_UIIngredientInfoWnd[];
extern const uintptr_t bases_UIIngredientInfoWnd[];
extern const uintptr_t base_UIIngredientInfoWnd[];
extern const uintptr_t locator_UILapineBox_IngredientListWnd[];
extern const uintptr_t hierarchy_UILapineBox_IngredientListWnd[];
extern const uintptr_t bases_UILapineBox_IngredientListWnd[];
extern const uintptr_t base_UILapineBox_IngredientListWnd[];
extern const uintptr_t base_UIScrollWnd[];
extern const uintptr_t hierarchy_UIScrollWnd[];
extern const uintptr_t bases_UIScrollWnd[];
extern const uintptr_t locator_UILapineBoxWnd[];
extern const uintptr_t hierarchy_UILapineBoxWnd[];
extern const uintptr_t bases_UILapineBoxWnd[];
extern const uintptr_t base_UILapineBoxWnd[];
extern const uintptr_t locator_UILapineUpgradeBox_IngredientInfoWnd[];
extern const uintptr_t hierarchy_UILapineUpgradeBox_IngredientInfoWnd[];
extern const uintptr_t bases_UILapineUpgradeBox_IngredientInfoWnd[];
extern const uintptr_t base_UILapineUpgradeBox_IngredientInfoWnd[];
extern const uintptr_t locator_UILapineUpgradeBox_IngredientListWnd[];
extern const uintptr_t hierarchy_UILapineUpgradeBox_IngredientListWnd[];
extern const uintptr_t bases_UILapineUpgradeBox_IngredientListWnd[];
extern const uintptr_t base_UILapineUpgradeBox_IngredientListWnd[];
extern const uintptr_t locator_UILapineUpgradeBoxWnd[];
extern const uintptr_t hierarchy_UILapineUpgradeBoxWnd[];
extern const uintptr_t bases_UILapineUpgradeBoxWnd[];
extern const uintptr_t base_UILapineUpgradeBoxWnd[];
extern const uintptr_t locator_UIReputeWnd[];
extern const uintptr_t hierarchy_UIReputeWnd[];
extern const uintptr_t bases_UIReputeWnd[];
extern const uintptr_t base_UIReputeWnd[];
extern const uintptr_t locator_CAcidifiedThrow[];
extern const uintptr_t hierarchy_CAcidifiedThrow[];
extern const uintptr_t bases_CAcidifiedThrow[];
extern const uintptr_t base_CAcidifiedThrow[];
extern const uintptr_t base_CEffect[];
extern const uintptr_t hierarchy_CEffect[];
extern const uintptr_t bases_CEffect[];
extern const uintptr_t locator_CAxeBoomerang[];
extern const uintptr_t hierarchy_CAxeBoomerang[];
extern const uintptr_t bases_CAxeBoomerang[];
extern const uintptr_t base_CAxeBoomerang[];
extern const uintptr_t locator_CAnimatedEmitterEffect[];
extern const uintptr_t hierarchy_CAnimatedEmitterEffect[];
extern const uintptr_t bases_CAnimatedEmitterEffect[];
extern const uintptr_t base_CAnimatedEmitterEffect[];
extern const uintptr_t base_CEmitterEffect[];
extern const uintptr_t hierarchy_CEmitterEffect[];
extern const uintptr_t bases_CEmitterEffect[];
extern const uintptr_t locator_CCartCannon[];
extern const uintptr_t hierarchy_CCartCannon[];
extern const uintptr_t bases_CCartCannon[];
extern const uintptr_t base_CCartCannon[];
extern const uintptr_t locator_CBindTrapEffect[];
extern const uintptr_t hierarchy_CBindTrapEffect[];
extern const uintptr_t bases_CBindTrapEffect[];
extern const uintptr_t base_CBindTrapEffect[];
extern const uintptr_t locator_CCloud[];
extern const uintptr_t hierarchy_CCloud[];
extern const uintptr_t bases_CCloud[];
extern const uintptr_t base_CCloud[];
extern const uintptr_t locator_CCloudKillEffect[];
extern const uintptr_t hierarchy_CCloudKillEffect[];
extern const uintptr_t bases_CCloudKillEffect[];
extern const uintptr_t base_CCloudKillEffect[];
extern const uintptr_t locator_CCommonFloorEffect[];
extern const uintptr_t hierarchy_CCommonFloorEffect[];
extern const uintptr_t bases_CCommonFloorEffect[];
extern const uintptr_t base_CCommonFloorEffect[];
extern const uintptr_t locator_CEvilLandEffect[];
extern const uintptr_t hierarchy_CEvilLandEffect[];
extern const uintptr_t bases_CEvilLandEffect[];
extern const uintptr_t base_CEvilLandEffect[];
extern const uintptr_t locator_CEvilsPawEffect[];
extern const uintptr_t hierarchy_CEvilsPawEffect[];
extern const uintptr_t bases_CEvilsPawEffect[];
extern const uintptr_t base_CEvilsPawEffect[];
extern const uintptr_t locator_CFortuneKiss[];
extern const uintptr_t hierarchy_CFortuneKiss[];
extern const uintptr_t bases_CFortuneKiss[];
extern const uintptr_t base_CFortuneKiss[];
extern const uintptr_t locator_CFriggSongEffect[];
extern const uintptr_t hierarchy_CFriggSongEffect[];
extern const uintptr_t bases_CFriggSongEffect[];
extern const uintptr_t base_CFriggSongEffect[];
extern const uintptr_t locator_CChangeSizeEffect[];
extern const uintptr_t hierarchy_CChangeSizeEffect[];
extern const uintptr_t bases_CChangeSizeEffect[];
extern const uintptr_t base_CChangeSizeEffect[];
extern const uintptr_t locator_CChargingPierceEffect[];
extern const uintptr_t hierarchy_CChargingPierceEffect[];
extern const uintptr_t bases_CChargingPierceEffect[];
extern const uintptr_t base_CChargingPierceEffect[];
extern const uintptr_t locator_CHitLineEffect[];
extern const uintptr_t hierarchy_CHitLineEffect[];
extern const uintptr_t bases_CHitLineEffect[];
extern const uintptr_t base_CHitLineEffect[];
extern const uintptr_t locator_CMeteorEffect[];
extern const uintptr_t hierarchy_CMeteorEffect[];
extern const uintptr_t bases_CMeteorEffect[];
extern const uintptr_t base_CMeteorEffect[];
extern const uintptr_t locator_CNyanggrassEffect[];
extern const uintptr_t hierarchy_CNyanggrassEffect[];
extern const uintptr_t bases_CNyanggrassEffect[];
extern const uintptr_t base_CNyanggrassEffect[];
extern const uintptr_t locator_CPowderingEffect[];
extern const uintptr_t hierarchy_CPowderingEffect[];
extern const uintptr_t bases_CPowderingEffect[];
extern const uintptr_t base_CPowderingEffect[];
extern const uintptr_t locator_CRectUpEffect[];
extern const uintptr_t hierarchy_CRectUpEffect[];
extern const uintptr_t bases_CRectUpEffect[];
extern const uintptr_t base_CRectUpEffect[];
extern const uintptr_t locator_CRichManKim[];
extern const uintptr_t hierarchy_CRichManKim[];
extern const uintptr_t bases_CRichManKim[];
extern const uintptr_t base_CRichManKim[];
extern const uintptr_t locator_CRootTwistEffect[];
extern const uintptr_t hierarchy_CRootTwistEffect[];
extern const uintptr_t bases_CRootTwistEffect[];
extern const uintptr_t base_CRootTwistEffect[];
extern const uintptr_t locator_CSpriteMableEffect[];
extern const uintptr_t hierarchy_CSpriteMableEffect[];
extern const uintptr_t bases_CSpriteMableEffect[];
extern const uintptr_t base_CSpriteMableEffect[];
extern const uintptr_t locator_CStemSpearEffect[];
extern const uintptr_t hierarchy_CStemSpearEffect[];
extern const uintptr_t bases_CStemSpearEffect[];
extern const uintptr_t base_CStemSpearEffect[];
extern const uintptr_t locator_CSvgSpiritEffect[];
extern const uintptr_t hierarchy_CSvgSpiritEffect[];
extern const uintptr_t bases_CSvgSpiritEffect[];
extern const uintptr_t base_CSvgSpiritEffect[];
extern const uintptr_t locator_CTunaPartyEffect[];
extern const uintptr_t hierarchy_CTunaPartyEffect[];
extern const uintptr_t bases_CTunaPartyEffect[];
extern const uintptr_t base_CTunaPartyEffect[];
extern const uintptr_t locator_CWarningPlaneEffect[];
extern const uintptr_t hierarchy_CWarningPlaneEffect[];
extern const uintptr_t bases_CWarningPlaneEffect[];
extern const uintptr_t base_CWarningPlaneEffect[];
extern const uintptr_t locator_CFallenAngelEffect[];
extern const uintptr_t hierarchy_CFallenAngelEffect[];
extern const uintptr_t bases_CFallenAngelEffect[];
extern const uintptr_t base_CFallenAngelEffect[];
extern const uintptr_t locator_CGravityControlEffect[];
extern const uintptr_t hierarchy_CGravityControlEffect[];
extern const uintptr_t bases_CGravityControlEffect[];
extern const uintptr_t base_CGravityControlEffect[];
extern const uintptr_t locator_CJupitelThunderHit[];
extern const uintptr_t hierarchy_CJupitelThunderHit[];
extern const uintptr_t bases_CJupitelThunderHit[];
extern const uintptr_t base_CJupitelThunderHit[];
extern const uintptr_t locator_CNewCloudKillEffect[];
extern const uintptr_t hierarchy_CNewCloudKillEffect[];
extern const uintptr_t bases_CNewCloudKillEffect[];
extern const uintptr_t base_CNewCloudKillEffect[];
extern const uintptr_t locator_CRandomOptEffect[];
extern const uintptr_t hierarchy_CRandomOptEffect[];
extern const uintptr_t bases_CRandomOptEffect[];
extern const uintptr_t base_CRandomOptEffect[];
extern const uintptr_t locator_CRichsCoinEffect[];
extern const uintptr_t hierarchy_CRichsCoinEffect[];
extern const uintptr_t bases_CRichsCoinEffect[];
extern const uintptr_t base_CRichsCoinEffect[];
extern const uintptr_t locator_CServantWeaponEffect[];
extern const uintptr_t hierarchy_CServantWeaponEffect[];
extern const uintptr_t bases_CServantWeaponEffect[];
extern const uintptr_t base_CServantWeaponEffect[];
extern const uintptr_t locator_CSevereRainStorm[];
extern const uintptr_t hierarchy_CSevereRainStorm[];
extern const uintptr_t bases_CSevereRainStorm[];
extern const uintptr_t base_CSevereRainStorm[];
extern const uintptr_t locator_CShowEffect[];
extern const uintptr_t hierarchy_CShowEffect[];
extern const uintptr_t bases_CShowEffect[];
extern const uintptr_t base_CShowEffect[];
extern const uintptr_t locator_CSoulCollectEffect[];
extern const uintptr_t hierarchy_CSoulCollectEffect[];
extern const uintptr_t bases_CSoulCollectEffect[];
extern const uintptr_t base_CSoulCollectEffect[];
extern const uintptr_t locator_CSoulUnityEffect[];
extern const uintptr_t hierarchy_CSoulUnityEffect[];
extern const uintptr_t bases_CSoulUnityEffect[];
extern const uintptr_t base_CSoulUnityEffect[];
extern const uintptr_t locator_CStarEmperorEffect[];
extern const uintptr_t hierarchy_CStarEmperorEffect[];
extern const uintptr_t bases_CStarEmperorEffect[];
extern const uintptr_t base_CStarEmperorEffect[];
extern const uintptr_t locator_CSunStanceEffect[];
extern const uintptr_t hierarchy_CSunStanceEffect[];
extern const uintptr_t bases_CSunStanceEffect[];
extern const uintptr_t base_CSunStanceEffect[];
extern const uintptr_t locator_CTetraBallEffect[];
extern const uintptr_t hierarchy_CTetraBallEffect[];
extern const uintptr_t bases_CTetraBallEffect[];
extern const uintptr_t base_CTetraBallEffect[];
extern const uintptr_t locator_CTriangleShotArrow[];
extern const uintptr_t hierarchy_CTriangleShotArrow[];
extern const uintptr_t bases_CTriangleShotArrow[];
extern const uintptr_t base_CTriangleShotArrow[];
extern const uintptr_t locator_CBash[];
extern const uintptr_t hierarchy_CBash[];
extern const uintptr_t bases_CBash[];
extern const uintptr_t base_CBash[];
extern const uintptr_t locator_CCellRangeEffect[];
extern const uintptr_t hierarchy_CCellRangeEffect[];
extern const uintptr_t bases_CCellRangeEffect[];
extern const uintptr_t base_CCellRangeEffect[];
extern const uintptr_t locator_CCherryBlossomEffect[];
extern const uintptr_t hierarchy_CCherryBlossomEffect[];
extern const uintptr_t bases_CCherryBlossomEffect[];
extern const uintptr_t base_CCherryBlossomEffect[];
extern const uintptr_t locator_CGlassWall[];
extern const uintptr_t hierarchy_CGlassWall[];
extern const uintptr_t bases_CGlassWall[];
extern const uintptr_t base_CGlassWall[];
extern const uintptr_t locator_CHealSp[];
extern const uintptr_t hierarchy_CHealSp[];
extern const uintptr_t bases_CHealSp[];
extern const uintptr_t base_CHealSp[];
extern const uintptr_t locator_CLevel150Effect[];
extern const uintptr_t hierarchy_CLevel150Effect[];
extern const uintptr_t bases_CLevel150Effect[];
extern const uintptr_t base_CLevel150Effect[];
extern const uintptr_t locator_CLevel150SubEffect[];
extern const uintptr_t hierarchy_CLevel150SubEffect[];
extern const uintptr_t bases_CLevel150SubEffect[];
extern const uintptr_t base_CLevel150SubEffect[];
extern const uintptr_t locator_CMagicFloorEffect[];
extern const uintptr_t hierarchy_CMagicFloorEffect[];
extern const uintptr_t bases_CMagicFloorEffect[];
extern const uintptr_t base_CMagicFloorEffect[];
extern const uintptr_t locator_CMagnumBreak[];
extern const uintptr_t hierarchy_CMagnumBreak[];
extern const uintptr_t bases_CMagnumBreak[];
extern const uintptr_t base_CMagnumBreak[];
extern const uintptr_t locator_CMapChainEffect[];
extern const uintptr_t hierarchy_CMapChainEffect[];
extern const uintptr_t bases_CMapChainEffect[];
extern const uintptr_t base_CMapChainEffect[];
extern const uintptr_t locator_CMappillarEffect[];
extern const uintptr_t hierarchy_CMappillarEffect[];
extern const uintptr_t bases_CMappillarEffect[];
extern const uintptr_t base_CMappillarEffect[];
extern const uintptr_t locator_CSquareRangeEffect[];
extern const uintptr_t hierarchy_CSquareRangeEffect[];
extern const uintptr_t bases_CSquareRangeEffect[];
extern const uintptr_t base_CSquareRangeEffect[];
extern const uintptr_t locator_CSteal[];
extern const uintptr_t hierarchy_CSteal[];
extern const uintptr_t bases_CSteal[];
extern const uintptr_t base_CSteal[];
extern const uintptr_t locator_CEffect[];
extern const uintptr_t locator_CElectricEffect[];
extern const uintptr_t hierarchy_CElectricEffect[];
extern const uintptr_t bases_CElectricEffect[];
extern const uintptr_t base_CElectricEffect[];
extern const uintptr_t locator_CEmitterEffect[];
extern const uintptr_t locator_CEnergyOrbEffect[];
extern const uintptr_t hierarchy_CEnergyOrbEffect[];
extern const uintptr_t bases_CEnergyOrbEffect[];
extern const uintptr_t base_CEnergyOrbEffect[];
extern const uintptr_t locator_CEZ2STREffect[];
extern const uintptr_t hierarchy_CEZ2STREffect[];
extern const uintptr_t bases_CEZ2STREffect[];
extern const uintptr_t base_CEZ2STREffect[];
extern const uintptr_t locator_CFlowerCastingEffect[];
extern const uintptr_t hierarchy_CFlowerCastingEffect[];
extern const uintptr_t bases_CFlowerCastingEffect[];
extern const uintptr_t base_CFlowerCastingEffect[];
extern const uintptr_t locator_CHealEffect[];
extern const uintptr_t hierarchy_CHealEffect[];
extern const uintptr_t bases_CHealEffect[];
extern const uintptr_t base_CHealEffect[];
extern const uintptr_t locator_CLevel99Effect[];
extern const uintptr_t hierarchy_CLevel99Effect[];
extern const uintptr_t bases_CLevel99Effect[];
extern const uintptr_t base_CLevel99Effect[];
extern const uintptr_t locator_CLevel99Orb1Effect[];
extern const uintptr_t hierarchy_CLevel99Orb1Effect[];
extern const uintptr_t bases_CLevel99Orb1Effect[];
extern const uintptr_t base_CLevel99Orb1Effect[];
extern const uintptr_t locator_CLevel99Orb2Effect[];
extern const uintptr_t hierarchy_CLevel99Orb2Effect[];
extern const uintptr_t bases_CLevel99Orb2Effect[];
extern const uintptr_t base_CLevel99Orb2Effect[];
extern const uintptr_t locator_CLightSphereEffect[];
extern const uintptr_t hierarchy_CLightSphereEffect[];
extern const uintptr_t bases_CLightSphereEffect[];
extern const uintptr_t base_CLightSphereEffect[];
extern const uintptr_t locator_CMapMagicZoneEffect[];
extern const uintptr_t hierarchy_CMapMagicZoneEffect[];
extern const uintptr_t bases_CMapMagicZoneEffect[];
extern const uintptr_t base_CMapMagicZoneEffect[];
extern const uintptr_t locator_CPokJukEffect[];
extern const uintptr_t hierarchy_CPokJukEffect[];
extern const uintptr_t bases_CPokJukEffect[];
extern const uintptr_t base_CPokJukEffect[];
extern const uintptr_t locator_CRainbowEffect[];
extern const uintptr_t hierarchy_CRainbowEffect[];
extern const uintptr_t bases_CRainbowEffect[];
extern const uintptr_t base_CRainbowEffect[];
extern const uintptr_t locator_CRedLineEffect[];
extern const uintptr_t hierarchy_CRedLineEffect[];
extern const uintptr_t bases_CRedLineEffect[];
extern const uintptr_t base_CRedLineEffect[];
extern const uintptr_t locator_CRotateBgEffect[];
extern const uintptr_t hierarchy_CRotateBgEffect[];
extern const uintptr_t bases_CRotateBgEffect[];
extern const uintptr_t base_CRotateBgEffect[];
extern const uintptr_t locator_CRotateItemEffect[];
extern const uintptr_t hierarchy_CRotateItemEffect[];
extern const uintptr_t bases_CRotateItemEffect[];
extern const uintptr_t base_CRotateItemEffect[];
extern const uintptr_t locator_CRotateLineEffect[];
extern const uintptr_t hierarchy_CRotateLineEffect[];
extern const uintptr_t bases_CRotateLineEffect[];
extern const uintptr_t base_CRotateLineEffect[];
extern const uintptr_t locator_CSaintCastingEffect[];
extern const uintptr_t hierarchy_CSaintCastingEffect[];
extern const uintptr_t bases_CSaintCastingEffect[];
extern const uintptr_t base_CSaintCastingEffect[];
extern const uintptr_t locator_CScatterEffect[];
extern const uintptr_t hierarchy_CScatterEffect[];
extern const uintptr_t bases_CScatterEffect[];
extern const uintptr_t base_CScatterEffect[];
extern const uintptr_t locator_CSmokeEffect[];
extern const uintptr_t hierarchy_CSmokeEffect[];
extern const uintptr_t bases_CSmokeEffect[];
extern const uintptr_t base_CSmokeEffect[];
extern const uintptr_t locator_CSphereWind2Effect[];
extern const uintptr_t hierarchy_CSphereWind2Effect[];
extern const uintptr_t bases_CSphereWind2Effect[];
extern const uintptr_t base_CSphereWind2Effect[];
extern const uintptr_t locator_CSphereWindEffect[];
extern const uintptr_t hierarchy_CSphereWindEffect[];
extern const uintptr_t bases_CSphereWindEffect[];
extern const uintptr_t base_CSphereWindEffect[];
extern const uintptr_t locator_CSpriteEffect[];
extern const uintptr_t hierarchy_CSpriteEffect[];
extern const uintptr_t bases_CSpriteEffect[];
extern const uintptr_t base_CSpriteEffect[];
extern const uintptr_t locator_CStarEffect[];
extern const uintptr_t hierarchy_CStarEffect[];
extern const uintptr_t bases_CStarEffect[];
extern const uintptr_t base_CStarEffect[];
extern const uintptr_t locator_CStormGustEffect[];
extern const uintptr_t hierarchy_CStormGustEffect[];
extern const uintptr_t bases_CStormGustEffect[];
extern const uintptr_t base_CStormGustEffect[];
extern const uintptr_t locator_CStormKickEffect[];
extern const uintptr_t hierarchy_CStormKickEffect[];
extern const uintptr_t bases_CStormKickEffect[];
extern const uintptr_t base_CStormKickEffect[];
extern const uintptr_t locator_CThrowItemEffect[];
extern const uintptr_t hierarchy_CThrowItemEffect[];
extern const uintptr_t bases_CThrowItemEffect[];
extern const uintptr_t base_CThrowItemEffect[];
extern const uintptr_t locator_CVolcanoEffect[];
extern const uintptr_t hierarchy_CVolcanoEffect[];
extern const uintptr_t bases_CVolcanoEffect[];
extern const uintptr_t base_CVolcanoEffect[];
extern const uintptr_t locator_CAstralStrikeEffect[];
extern const uintptr_t hierarchy_CAstralStrikeEffect[];
extern const uintptr_t bases_CAstralStrikeEffect[];
extern const uintptr_t base_CAstralStrikeEffect[];
extern const uintptr_t locator_CAMachineEffect[];
extern const uintptr_t hierarchy_CAMachineEffect[];
extern const uintptr_t bases_CAMachineEffect[];
extern const uintptr_t base_CAMachineEffect[];
extern const uintptr_t locator_CBlastForge[];
extern const uintptr_t hierarchy_CBlastForge[];
extern const uintptr_t bases_CBlastForge[];
extern const uintptr_t base_CBlastForge[];
extern const uintptr_t locator_CCaneOfEvilEyeEffect[];
extern const uintptr_t hierarchy_CCaneOfEvilEyeEffect[];
extern const uintptr_t bases_CCaneOfEvilEyeEffect[];
extern const uintptr_t base_CCaneOfEvilEyeEffect[];
extern const uintptr_t locator_CEZ2STREffectEx[];
extern const uintptr_t hierarchy_CEZ2STREffectEx[];
extern const uintptr_t bases_CEZ2STREffectEx[];
extern const uintptr_t base_CEZ2STREffectEx[];
extern const uintptr_t locator_CFuumaShouakuEffect[];
extern const uintptr_t hierarchy_CFuumaShouakuEffect[];
extern const uintptr_t bases_CFuumaShouakuEffect[];
extern const uintptr_t base_CFuumaShouakuEffect[];
extern const uintptr_t locator_CJupitelThunderStormEffect[];
extern const uintptr_t hierarchy_CJupitelThunderStormEffect[];
extern const uintptr_t bases_CJupitelThunderStormEffect[];
extern const uintptr_t base_CJupitelThunderStormEffect[];
extern const uintptr_t locator_CClimaxEffect[];
extern const uintptr_t hierarchy_CClimaxEffect[];
extern const uintptr_t bases_CClimaxEffect[];
extern const uintptr_t base_CClimaxEffect[];
extern const uintptr_t locator_CCompetentia[];
extern const uintptr_t hierarchy_CCompetentia[];
extern const uintptr_t bases_CCompetentia[];
extern const uintptr_t base_CCompetentia[];
extern const uintptr_t locator_CCresciveBolt[];
extern const uintptr_t hierarchy_CCresciveBolt[];
extern const uintptr_t bases_CCresciveBolt[];
extern const uintptr_t base_CCresciveBolt[];
extern const uintptr_t locator_CCrimsonArrowEffect[];
extern const uintptr_t hierarchy_CCrimsonArrowEffect[];
extern const uintptr_t bases_CCrimsonArrowEffect[];
extern const uintptr_t base_CCrimsonArrowEffect[];
extern const uintptr_t locator_CCurseOfCubeEffect[];
extern const uintptr_t hierarchy_CCurseOfCubeEffect[];
extern const uintptr_t bases_CCurseOfCubeEffect[];
extern const uintptr_t base_CCurseOfCubeEffect[];
extern const uintptr_t locator_CCylinderEffect[];
extern const uintptr_t hierarchy_CCylinderEffect[];
extern const uintptr_t bases_CCylinderEffect[];
extern const uintptr_t base_CCylinderEffect[];
extern const uintptr_t locator_CDancingKnifeEffect[];
extern const uintptr_t hierarchy_CDancingKnifeEffect[];
extern const uintptr_t bases_CDancingKnifeEffect[];
extern const uintptr_t base_CDancingKnifeEffect[];
extern const uintptr_t locator_CEternalSlashAttackEffect[];
extern const uintptr_t hierarchy_CEternalSlashAttackEffect[];
extern const uintptr_t bases_CEternalSlashAttackEffect[];
extern const uintptr_t base_CEternalSlashAttackEffect[];
extern const uintptr_t locator_CEternalSlashCountEffect[];
extern const uintptr_t hierarchy_CEternalSlashCountEffect[];
extern const uintptr_t bases_CEternalSlashCountEffect[];
extern const uintptr_t base_CEternalSlashCountEffect[];
extern const uintptr_t locator_CFloralFlareRoad[];
extern const uintptr_t hierarchy_CFloralFlareRoad[];
extern const uintptr_t bases_CFloralFlareRoad[];
extern const uintptr_t base_CFloralFlareRoad[];
extern const uintptr_t locator_CFromTheAbyssEffect[];
extern const uintptr_t hierarchy_CFromTheAbyssEffect[];
extern const uintptr_t bases_CFromTheAbyssEffect[];
extern const uintptr_t base_CFromTheAbyssEffect[];
extern const uintptr_t locator_CFullScreenEffect[];
extern const uintptr_t hierarchy_CFullScreenEffect[];
extern const uintptr_t bases_CFullScreenEffect[];
extern const uintptr_t base_CFullScreenEffect[];
extern const uintptr_t locator_CGaleStorm[];
extern const uintptr_t hierarchy_CGaleStorm[];
extern const uintptr_t bases_CGaleStorm[];
extern const uintptr_t base_CGaleStorm[];
extern const uintptr_t locator_CHawkVumerangEffect[];
extern const uintptr_t hierarchy_CHawkVumerangEffect[];
extern const uintptr_t bases_CHawkVumerangEffect[];
extern const uintptr_t base_CHawkVumerangEffect[];
extern const uintptr_t locator_CItemDropEffect[];
extern const uintptr_t hierarchy_CItemDropEffect[];
extern const uintptr_t bases_CItemDropEffect[];
extern const uintptr_t base_CItemDropEffect[];
extern const uintptr_t locator_CPetitioEffect[];
extern const uintptr_t hierarchy_CPetitioEffect[];
extern const uintptr_t bases_CPetitioEffect[];
extern const uintptr_t base_CPetitioEffect[];
extern const uintptr_t locator_CPneumaticusProcellaEffect[];
extern const uintptr_t hierarchy_CPneumaticusProcellaEffect[];
extern const uintptr_t bases_CPneumaticusProcellaEffect[];
extern const uintptr_t base_CPneumaticusProcellaEffect[];
extern const uintptr_t locator_CRandomHitEffect[];
extern const uintptr_t hierarchy_CRandomHitEffect[];
extern const uintptr_t bases_CRandomHitEffect[];
extern const uintptr_t base_CRandomHitEffect[];
extern const uintptr_t locator_CSavageImpactEffect[];
extern const uintptr_t hierarchy_CSavageImpactEffect[];
extern const uintptr_t bases_CSavageImpactEffect[];
extern const uintptr_t base_CSavageImpactEffect[];
extern const uintptr_t locator_CShadowExceed[];
extern const uintptr_t hierarchy_CShadowExceed[];
extern const uintptr_t bases_CShadowExceed[];
extern const uintptr_t base_CShadowExceed[];
extern const uintptr_t locator_CStormCannonEffect[];
extern const uintptr_t hierarchy_CStormCannonEffect[];
extern const uintptr_t bases_CStormCannonEffect[];
extern const uintptr_t base_CStormCannonEffect[];
extern const uintptr_t locator_CTalismanMarkEffect[];
extern const uintptr_t hierarchy_CTalismanMarkEffect[];
extern const uintptr_t bases_CTalismanMarkEffect[];
extern const uintptr_t base_CTalismanMarkEffect[];
extern const uintptr_t locator_CViolentQuakeEffect[];
extern const uintptr_t hierarchy_CViolentQuakeEffect[];
extern const uintptr_t bases_CViolentQuakeEffect[];
extern const uintptr_t base_CViolentQuakeEffect[];
extern const uintptr_t locator_CWindEffect[];
extern const uintptr_t hierarchy_CWindEffect[];
extern const uintptr_t bases_CWindEffect[];
extern const uintptr_t base_CWindEffect[];
extern const uintptr_t locator_UIDragMiniPartyWnd[];
extern const uintptr_t hierarchy_UIDragMiniPartyWnd[];
extern const uintptr_t bases_UIDragMiniPartyWnd[];
extern const uintptr_t base_UIDragMiniPartyWnd[];
extern const uintptr_t locator_UIFriendOptionWnd[];
extern const uintptr_t hierarchy_UIFriendOptionWnd[];
extern const uintptr_t bases_UIFriendOptionWnd[];
extern const uintptr_t base_UIFriendOptionWnd[];
extern const uintptr_t locator_UIJobIconButton[];
extern const uintptr_t hierarchy_UIJobIconButton[];
extern const uintptr_t bases_UIJobIconButton[];
extern const uintptr_t base_UIJobIconButton[];
extern const uintptr_t locator_UIMessengerGroupWnd[];
extern const uintptr_t hierarchy_UIMessengerGroupWnd[];
extern const uintptr_t bases_UIMessengerGroupWnd[];
extern const uintptr_t base_UIMessengerGroupWnd[];
extern const uintptr_t locator_UIMiniPartyInfoWnd[];
extern const uintptr_t hierarchy_UIMiniPartyInfoWnd[];
extern const uintptr_t bases_UIMiniPartyInfoWnd[];
extern const uintptr_t base_UIMiniPartyInfoWnd[];
extern const uintptr_t locator_UIMiniPartyWnd[];
extern const uintptr_t hierarchy_UIMiniPartyWnd[];
extern const uintptr_t bases_UIMiniPartyWnd[];
extern const uintptr_t base_UIMiniPartyWnd[];
extern const uintptr_t locator_CActRes[];
extern const uintptr_t hierarchy_CActRes[];
extern const uintptr_t bases_CActRes[];
extern const uintptr_t base_CActRes[];
extern const uintptr_t locator_C3dAttr[];
extern const uintptr_t hierarchy_C3dAttr[];
extern const uintptr_t bases_C3dAttr[];
extern const uintptr_t base_C3dAttr[];
extern const uintptr_t locator_CBitmapRes[];
extern const uintptr_t hierarchy_CBitmapRes[];
extern const uintptr_t bases_CBitmapRes[];
extern const uintptr_t base_CBitmapRes[];
extern const uintptr_t locator_CGifRes[];
extern const uintptr_t hierarchy_CGifRes[];
extern const uintptr_t bases_CGifRes[];
extern const uintptr_t base_CGifRes[];
extern const uintptr_t locator_CEmblemRes[];
extern const uintptr_t hierarchy_CEmblemRes[];
extern const uintptr_t bases_CEmblemRes[];
extern const uintptr_t base_CEmblemRes[];
extern const uintptr_t locator_CEZeffectRes[];
extern const uintptr_t hierarchy_CEZeffectRes[];
extern const uintptr_t bases_CEZeffectRes[];
extern const uintptr_t base_CEZeffectRes[];
extern const uintptr_t locator_CGndRes[];
extern const uintptr_t hierarchy_CGndRes[];
extern const uintptr_t bases_CGndRes[];
extern const uintptr_t base_CGndRes[];
extern const uintptr_t locator_CImfRes[];
extern const uintptr_t hierarchy_CImfRes[];
extern const uintptr_t bases_CImfRes[];
extern const uintptr_t base_CImfRes[];
extern const uintptr_t hierarchy_C3dModelRes[];
extern const uintptr_t bases_C3dModelRes[];
extern const uintptr_t base_C3dModelRes[];
extern const uintptr_t locator_C3dModelResV1[];
extern const uintptr_t hierarchy_C3dModelResV1[];
extern const uintptr_t bases_C3dModelResV1[];
extern const uintptr_t base_C3dModelResV1[];
extern const uintptr_t locator_C3dModelResV2[];
extern const uintptr_t hierarchy_C3dModelResV2[];
extern const uintptr_t bases_C3dModelResV2[];
extern const uintptr_t base_C3dModelResV2[];
extern const uintptr_t locator_C3dAnimModelRes[];
extern const uintptr_t hierarchy_C3dAnimModelRes[];
extern const uintptr_t bases_C3dAnimModelRes[];
extern const uintptr_t base_C3dAnimModelRes[];
extern const uintptr_t locator_C3dGrannyModelRes[];
extern const uintptr_t hierarchy_C3dGrannyModelRes[];
extern const uintptr_t bases_C3dGrannyModelRes[];
extern const uintptr_t base_C3dGrannyModelRes[];
extern const uintptr_t locator_C3dModelRes[];
extern const uintptr_t locator_CPaletteRes[];
extern const uintptr_t hierarchy_CPaletteRes[];
extern const uintptr_t bases_CPaletteRes[];
extern const uintptr_t base_CPaletteRes[];
extern const uintptr_t locator_CSprRes[];
extern const uintptr_t hierarchy_CSprRes[];
extern const uintptr_t bases_CSprRes[];
extern const uintptr_t base_CSprRes[];
extern const uintptr_t locator_CWave[];
extern const uintptr_t hierarchy_CWave[];
extern const uintptr_t bases_CWave[];
extern const uintptr_t base_CWave[];
extern const uintptr_t locator_C3dWorldRes[];
extern const uintptr_t hierarchy_C3dWorldRes[];
extern const uintptr_t bases_C3dWorldRes[];
extern const uintptr_t base_C3dWorldRes[];
extern const uintptr_t locator_UIAchScrollBar[];
extern const uintptr_t hierarchy_UIAchScrollBar[];
extern const uintptr_t bases_UIAchScrollBar[];
extern const uintptr_t base_UIAchScrollBar[];
extern const uintptr_t locator_UIAchTV[];
extern const uintptr_t hierarchy_UIAchTV[];
extern const uintptr_t bases_UIAchTV[];
extern const uintptr_t base_UIAchTV[];
extern const uintptr_t locator_UIAchTab[];
extern const uintptr_t hierarchy_UIAchTab[];
extern const uintptr_t bases_UIAchTab[];
extern const uintptr_t base_UIAchTab[];
extern const uintptr_t locator_UICustomGageBar[];
extern const uintptr_t hierarchy_UICustomGageBar[];
extern const uintptr_t bases_UICustomGageBar[];
extern const uintptr_t base_UICustomGageBar[];
extern const uintptr_t locator_UIAchUpperWnd[];
extern const uintptr_t hierarchy_UIAchUpperWnd[];
extern const uintptr_t bases_UIAchUpperWnd[];
extern const uintptr_t base_UIAchUpperWnd[];
extern const uintptr_t locator_UIToolTipBitmapWnd[];
extern const uintptr_t hierarchy_UIToolTipBitmapWnd[];
extern const uintptr_t bases_UIToolTipBitmapWnd[];
extern const uintptr_t base_UIToolTipBitmapWnd[];
extern const uintptr_t locator_UIAchViewWnd[];
extern const uintptr_t hierarchy_UIAchViewWnd[];
extern const uintptr_t bases_UIAchViewWnd[];
extern const uintptr_t base_UIAchViewWnd[];
extern const uintptr_t locator_UIAchListWnd[];
extern const uintptr_t hierarchy_UIAchListWnd[];
extern const uintptr_t bases_UIAchListWnd[];
extern const uintptr_t base_UIAchListWnd[];
extern const uintptr_t locator_UIAchSummaryWnd[];
extern const uintptr_t hierarchy_UIAchSummaryWnd[];
extern const uintptr_t bases_UIAchSummaryWnd[];
extern const uintptr_t base_UIAchSummaryWnd[];
extern const uintptr_t locator_UIAchievementWnd[];
extern const uintptr_t hierarchy_UIAchievementWnd[];
extern const uintptr_t bases_UIAchievementWnd[];
extern const uintptr_t base_UIAchievementWnd[];
extern const uintptr_t locator_UIAchTracingWnd[];
extern const uintptr_t hierarchy_UIAchTracingWnd[];
extern const uintptr_t bases_UIAchTracingWnd[];
extern const uintptr_t base_UIAchTracingWnd[];
extern const uintptr_t locator_UIJobSlotButton[];
extern const uintptr_t hierarchy_UIJobSlotButton[];
extern const uintptr_t bases_UIJobSlotButton[];
extern const uintptr_t base_UIJobSlotButton[];
extern const uintptr_t locator_UIRegisterPartyWnd[];
extern const uintptr_t hierarchy_UIRegisterPartyWnd[];
extern const uintptr_t bases_UIRegisterPartyWnd[];
extern const uintptr_t base_UIRegisterPartyWnd[];
extern const uintptr_t locator_UIApplyForPartyWnd[];
extern const uintptr_t hierarchy_UIApplyForPartyWnd[];
extern const uintptr_t bases_UIApplyForPartyWnd[];
extern const uintptr_t base_UIApplyForPartyWnd[];
extern const uintptr_t locator_UIPartyInfoWnd[];
extern const uintptr_t hierarchy_UIPartyInfoWnd[];
extern const uintptr_t bases_UIPartyInfoWnd[];
extern const uintptr_t base_UIPartyInfoWnd[];
extern const uintptr_t locator_UIAdvenPartyBoardWnd[];
extern const uintptr_t hierarchy_UIAdvenPartyBoardWnd[];
extern const uintptr_t bases_UIAdvenPartyBoardWnd[];
extern const uintptr_t base_UIAdvenPartyBoardWnd[];
extern const uintptr_t locator_UIRequestJoinPartyWnd[];
extern const uintptr_t hierarchy_UIRequestJoinPartyWnd[];
extern const uintptr_t bases_UIRequestJoinPartyWnd[];
extern const uintptr_t base_UIRequestJoinPartyWnd[];
extern const uintptr_t locator_CMergeItemWnd[];
extern const uintptr_t hierarchy_CMergeItemWnd[];
extern const uintptr_t bases_CMergeItemWnd[];
extern const uintptr_t base_CMergeItemWnd[];
extern const uintptr_t locator_UIBartermarketWnd[];
extern const uintptr_t hierarchy_UIBartermarketWnd[];
extern const uintptr_t bases_UIBartermarketWnd[];
extern const uintptr_t base_UIBartermarketWnd[];
extern const uintptr_t locator_UIBarterItemPurchaseWnd[];
extern const uintptr_t hierarchy_UIBarterItemPurchaseWnd[];
extern const uintptr_t bases_UIBarterItemPurchaseWnd[];
extern const uintptr_t base_UIBarterItemPurchaseWnd[];
extern const uintptr_t locator_UIPurchaseButton[];
extern const uintptr_t hierarchy_UIPurchaseButton[];
extern const uintptr_t bases_UIPurchaseButton[];
extern const uintptr_t base_UIPurchaseButton[];
extern const uintptr_t locator_UIExpandedBartermarketWnd[];
extern const uintptr_t hierarchy_UIExpandedBartermarketWnd[];
extern const uintptr_t bases_UIExpandedBartermarketWnd[];
extern const uintptr_t base_UIExpandedBartermarketWnd[];
extern const uintptr_t locator_UIExpandedBarterItemPurchaseWnd[];
extern const uintptr_t hierarchy_UIExpandedBarterItemPurchaseWnd[];
extern const uintptr_t bases_UIExpandedBarterItemPurchaseWnd[];
extern const uintptr_t base_UIExpandedBarterItemPurchaseWnd[];
extern const uintptr_t locator_UIExpandedBarterPurchaseButton[];
extern const uintptr_t hierarchy_UIExpandedBarterPurchaseButton[];
extern const uintptr_t bases_UIExpandedBarterPurchaseButton[];
extern const uintptr_t base_UIExpandedBarterPurchaseButton[];
extern const uintptr_t locator_UIMenuIcon[];
extern const uintptr_t hierarchy_UIMenuIcon[];
extern const uintptr_t bases_UIMenuIcon[];
extern const uintptr_t base_UIMenuIcon[];
extern const uintptr_t locator_UIMenuIconWnd[];
extern const uintptr_t hierarchy_UIMenuIconWnd[];
extern const uintptr_t bases_UIMenuIconWnd[];
extern const uintptr_t base_UIMenuIconWnd[];
extern const uintptr_t locator_UIClanInfoManageWnd[];
extern const uintptr_t hierarchy_UIClanInfoManageWnd[];
extern const uintptr_t bases_UIClanInfoManageWnd[];
extern const uintptr_t base_UIClanInfoManageWnd[];
extern const uintptr_t base_UIClanWnd[];
extern const uintptr_t hierarchy_UIClanWnd[];
extern const uintptr_t bases_UIClanWnd[];
extern const uintptr_t locator_UIClanWnd[];
extern const uintptr_t locator_UIEmotionListWnd[];
extern const uintptr_t hierarchy_UIEmotionListWnd[];
extern const uintptr_t bases_UIEmotionListWnd[];
extern const uintptr_t base_UIEmotionListWnd[];
extern const uintptr_t locator_UIBarGraph4[];
extern const uintptr_t hierarchy_UIBarGraph4[];
extern const uintptr_t bases_UIBarGraph4[];
extern const uintptr_t base_UIBarGraph4[];
extern const uintptr_t locator_UILoginWnd[];
extern const uintptr_t hierarchy_UILoginWnd[];
extern const uintptr_t bases_UILoginWnd[];
extern const uintptr_t base_UILoginWnd[];
extern const uintptr_t locator_UISelectServerWnd[];
extern const uintptr_t hierarchy_UISelectServerWnd[];
extern const uintptr_t bases_UISelectServerWnd[];
extern const uintptr_t base_UISelectServerWnd[];
extern const uintptr_t locator_UIMakeCharWnd[];
extern const uintptr_t hierarchy_UIMakeCharWnd[];
extern const uintptr_t bases_UIMakeCharWnd[];
extern const uintptr_t base_UIMakeCharWnd[];
extern const uintptr_t locator_UIFrameWnd[];
extern const uintptr_t locator_UIMessageBox[];
extern const uintptr_t hierarchy_UIMessageBox[];
extern const uintptr_t bases_UIMessageBox[];
extern const uintptr_t base_UIMessageBox[];
extern const uintptr_t locator_UIMessageBoxAutoreturn[];
extern const uintptr_t hierarchy_UIMessageBoxAutoreturn[];
extern const uintptr_t bases_UIMessageBoxAutoreturn[];
extern const uintptr_t base_UIMessageBoxAutoreturn[];
extern const uintptr_t locator_UILoginOTPWnd[];
extern const uintptr_t hierarchy_UILoginOTPWnd[];
extern const uintptr_t bases_UILoginOTPWnd[];
extern const uintptr_t base_UILoginOTPWnd[];
extern const uintptr_t locator_UINoticeWnd[];
extern const uintptr_t hierarchy_UINoticeWnd[];
extern const uintptr_t bases_UINoticeWnd[];
extern const uintptr_t base_UINoticeWnd[];
extern const uintptr_t locator_UICntWnd[];
extern const uintptr_t hierarchy_UICntWnd[];
extern const uintptr_t bases_UICntWnd[];
extern const uintptr_t base_UICntWnd[];
extern const uintptr_t locator_UIAllyGuildAcceptWnd[];
extern const uintptr_t hierarchy_UIAllyGuildAcceptWnd[];
extern const uintptr_t bases_UIAllyGuildAcceptWnd[];
extern const uintptr_t base_UIAllyGuildAcceptWnd[];
extern const uintptr_t locator_UIChatRoomWnd[];
extern const uintptr_t hierarchy_UIChatRoomWnd[];
extern const uintptr_t bases_UIChatRoomWnd[];
extern const uintptr_t base_UIChatRoomWnd[];
extern const uintptr_t locator_UIMonsterInfoWnd[];
extern const uintptr_t hierarchy_UIMonsterInfoWnd[];
extern const uintptr_t bases_UIMonsterInfoWnd[];
extern const uintptr_t base_UIMonsterInfoWnd[];
extern const uintptr_t locator_UITalkboxTrapInputWnd[];
extern const uintptr_t hierarchy_UITalkboxTrapInputWnd[];
extern const uintptr_t bases_UITalkboxTrapInputWnd[];
extern const uintptr_t base_UITalkboxTrapInputWnd[];
extern const uintptr_t locator_UIPetInfoWnd[];
extern const uintptr_t hierarchy_UIPetInfoWnd[];
extern const uintptr_t bases_UIPetInfoWnd[];
extern const uintptr_t base_UIPetInfoWnd[];
extern const uintptr_t locator_UIHomunInfoWnd[];
extern const uintptr_t hierarchy_UIHomunInfoWnd[];
extern const uintptr_t bases_UIHomunInfoWnd[];
extern const uintptr_t base_UIHomunInfoWnd[];
extern const uintptr_t locator_UIMerInfoWnd[];
extern const uintptr_t hierarchy_UIMerInfoWnd[];
extern const uintptr_t bases_UIMerInfoWnd[];
extern const uintptr_t base_UIMerInfoWnd[];
extern const uintptr_t locator_UIReplayButton[];
extern const uintptr_t hierarchy_UIReplayButton[];
extern const uintptr_t bases_UIReplayButton[];
extern const uintptr_t base_UIReplayButton[];
extern const uintptr_t locator_UIReplayControlWnd[];
extern const uintptr_t hierarchy_UIReplayControlWnd[];
extern const uintptr_t bases_UIReplayControlWnd[];
extern const uintptr_t base_UIReplayControlWnd[];
extern const uintptr_t locator_UIReplayListBox[];
extern const uintptr_t hierarchy_UIReplayListBox[];
extern const uintptr_t bases_UIReplayListBox[];
extern const uintptr_t base_UIReplayListBox[];
extern const uintptr_t locator_UISelectReplayDataWnd[];
extern const uintptr_t hierarchy_UISelectReplayDataWnd[];
extern const uintptr_t bases_UISelectReplayDataWnd[];
extern const uintptr_t base_UISelectReplayDataWnd[];
extern const uintptr_t locator_UIReplayRECControlWnd[];
extern const uintptr_t hierarchy_UIReplayRECControlWnd[];
extern const uintptr_t bases_UIReplayRECControlWnd[];
extern const uintptr_t base_UIReplayRECControlWnd[];
extern const uintptr_t locator_UIBank_NewWnd[];
extern const uintptr_t hierarchy_UIBank_NewWnd[];
extern const uintptr_t bases_UIBank_NewWnd[];
extern const uintptr_t base_UIBank_NewWnd[];
extern const uintptr_t locator_UINormalMB[];
extern const uintptr_t hierarchy_UINormalMB[];
extern const uintptr_t bases_UINormalMB[];
extern const uintptr_t base_UINormalMB[];
extern const uintptr_t locator_UIQuestDisplay[];
extern const uintptr_t hierarchy_UIQuestDisplay[];
extern const uintptr_t bases_UIQuestDisplay[];
extern const uintptr_t base_UIQuestDisplay[];
extern const uintptr_t locator_UIGuild_Storage_Log[];
extern const uintptr_t hierarchy_UIGuild_Storage_Log[];
extern const uintptr_t bases_UIGuild_Storage_Log[];
extern const uintptr_t base_UIGuild_Storage_Log[];
extern const uintptr_t locator_UIOpenMailBoxWnd[];
extern const uintptr_t hierarchy_UIOpenMailBoxWnd[];
extern const uintptr_t bases_UIOpenMailBoxWnd[];
extern const uintptr_t base_UIOpenMailBoxWnd[];
extern const uintptr_t locator_UISecondCostumeWnd[];
extern const uintptr_t hierarchy_UISecondCostumeWnd[];
extern const uintptr_t bases_UISecondCostumeWnd[];
extern const uintptr_t base_UISecondCostumeWnd[];
extern const uintptr_t locator_UIMileageWnd[];
extern const uintptr_t hierarchy_UIMileageWnd[];
extern const uintptr_t bases_UIMileageWnd[];
extern const uintptr_t base_UIMileageWnd[];
extern const uintptr_t locator_UIOpenRoulletteWnd[];
extern const uintptr_t hierarchy_UIOpenRoulletteWnd[];
extern const uintptr_t bases_UIOpenRoulletteWnd[];
extern const uintptr_t base_UIOpenRoulletteWnd[];
extern const uintptr_t locator_UITransMenuWnd[];
extern const uintptr_t hierarchy_UITransMenuWnd[];
extern const uintptr_t bases_UITransMenuWnd[];
extern const uintptr_t base_UITransMenuWnd[];
extern const uintptr_t locator_UIShowWarpWnd[];
extern const uintptr_t hierarchy_UIShowWarpWnd[];
extern const uintptr_t bases_UIShowWarpWnd[];
extern const uintptr_t base_UIShowWarpWnd[];
extern const uintptr_t locator_UIStaticText_LF[];
extern const uintptr_t hierarchy_UIStaticText_LF[];
extern const uintptr_t bases_UIStaticText_LF[];
extern const uintptr_t base_UIStaticText_LF[];
extern const uintptr_t locator_UINoteWnd_Scroll[];
extern const uintptr_t hierarchy_UINoteWnd_Scroll[];
extern const uintptr_t bases_UINoteWnd_Scroll[];
extern const uintptr_t base_UINoteWnd_Scroll[];
extern const uintptr_t locator_UINoteWnd[];
extern const uintptr_t hierarchy_UINoteWnd[];
extern const uintptr_t bases_UINoteWnd[];
extern const uintptr_t base_UINoteWnd[];
extern const uintptr_t locator_UIRPImageWnd2[];
extern const uintptr_t hierarchy_UIRPImageWnd2[];
extern const uintptr_t bases_UIRPImageWnd2[];
extern const uintptr_t base_UIRPImageWnd2[];
extern const uintptr_t locator_UIAccountLimitedToolWnd[];
extern const uintptr_t hierarchy_UIAccountLimitedToolWnd[];
extern const uintptr_t bases_UIAccountLimitedToolWnd[];
extern const uintptr_t base_UIAccountLimitedToolWnd[];
extern const uintptr_t locator_UIHotkeyGuideWnd[];
extern const uintptr_t hierarchy_UIHotkeyGuideWnd[];
extern const uintptr_t bases_UIHotkeyGuideWnd[];
extern const uintptr_t base_UIHotkeyGuideWnd[];
extern const uintptr_t locator_UIIllustWnd[];
extern const uintptr_t hierarchy_UIIllustWnd[];
extern const uintptr_t bases_UIIllustWnd[];
extern const uintptr_t base_UIIllustWnd[];
extern const uintptr_t locator_UIKeyStrokeWnd[];
extern const uintptr_t hierarchy_UIKeyStrokeWnd[];
extern const uintptr_t bases_UIKeyStrokeWnd[];
extern const uintptr_t base_UIKeyStrokeWnd[];
extern const uintptr_t locator_UICompositionWnd[];
extern const uintptr_t hierarchy_UICompositionWnd[];
extern const uintptr_t bases_UICompositionWnd[];
extern const uintptr_t base_UICompositionWnd[];
extern const uintptr_t locator_UICandidateWnd[];
extern const uintptr_t hierarchy_UICandidateWnd[];
extern const uintptr_t bases_UICandidateWnd[];
extern const uintptr_t base_UICandidateWnd[];
extern const uintptr_t locator_UIRoundBoxTooltipWnd[];
extern const uintptr_t hierarchy_UIRoundBoxTooltipWnd[];
extern const uintptr_t bases_UIRoundBoxTooltipWnd[];
extern const uintptr_t base_UIRoundBoxTooltipWnd[];
extern const uintptr_t locator_UIColoringTextViewer[];
extern const uintptr_t hierarchy_UIColoringTextViewer[];
extern const uintptr_t bases_UIColoringTextViewer[];
extern const uintptr_t base_UIColoringTextViewer[];
extern const uintptr_t locator_UIMiniMapWnd[];
extern const uintptr_t hierarchy_UIMiniMapWnd[];
extern const uintptr_t bases_UIMiniMapWnd[];
extern const uintptr_t base_UIMiniMapWnd[];
extern const uintptr_t locator_UIEditctrlWnd[];
extern const uintptr_t hierarchy_UIEditctrlWnd[];
extern const uintptr_t bases_UIEditctrlWnd[];
extern const uintptr_t base_UIEditctrlWnd[];
extern const uintptr_t locator_UIMerchantItemLogWnd[];
extern const uintptr_t hierarchy_UIMerchantItemLogWnd[];
extern const uintptr_t bases_UIMerchantItemLogWnd[];
extern const uintptr_t base_UIMerchantItemLogWnd[];
extern const uintptr_t locator_UIMakeTargetListWnd[];
extern const uintptr_t hierarchy_UIMakeTargetListWnd[];
extern const uintptr_t bases_UIMakeTargetListWnd[];
extern const uintptr_t base_UIMakeTargetListWnd[];
extern const uintptr_t locator_UIItemRepairWnd[];
extern const uintptr_t hierarchy_UIItemRepairWnd[];
extern const uintptr_t bases_UIItemRepairWnd[];
extern const uintptr_t base_UIItemRepairWnd[];
extern const uintptr_t locator_UIWeaponRefineWnd[];
extern const uintptr_t hierarchy_UIWeaponRefineWnd[];
extern const uintptr_t bases_UIWeaponRefineWnd[];
extern const uintptr_t base_UIWeaponRefineWnd[];
extern const uintptr_t locator_UIMakeTargetProcessWnd[];
extern const uintptr_t hierarchy_UIMakeTargetProcessWnd[];
extern const uintptr_t bases_UIMakeTargetProcessWnd[];
extern const uintptr_t base_UIMakeTargetProcessWnd[];
extern const uintptr_t locator_UIMakeTargetResultWnd[];
extern const uintptr_t hierarchy_UIMakeTargetResultWnd[];
extern const uintptr_t bases_UIMakeTargetResultWnd[];
extern const uintptr_t base_UIMakeTargetResultWnd[];
extern const uintptr_t locator_UIParaItemShopWnd[];
extern const uintptr_t hierarchy_UIParaItemShopWnd[];
extern const uintptr_t bases_UIParaItemShopWnd[];
extern const uintptr_t base_UIParaItemShopWnd[];
extern const uintptr_t locator_UIParaItemPurchaseWnd[];
extern const uintptr_t hierarchy_UIParaItemPurchaseWnd[];
extern const uintptr_t bases_UIParaItemPurchaseWnd[];
extern const uintptr_t base_UIParaItemPurchaseWnd[];
extern const uintptr_t locator_UIParaResultWnd[];
extern const uintptr_t hierarchy_UIParaResultWnd[];
extern const uintptr_t bases_UIParaResultWnd[];
extern const uintptr_t base_UIParaResultWnd[];
extern const uintptr_t locator_UIPetEvolutionWnd[];
extern const uintptr_t hierarchy_UIPetEvolutionWnd[];
extern const uintptr_t bases_UIPetEvolutionWnd[];
extern const uintptr_t base_UIPetEvolutionWnd[];
extern const uintptr_t locator_UIScrollWnd[];
extern const uintptr_t locator_UITagButton[];
extern const uintptr_t hierarchy_UITagButton[];
extern const uintptr_t bases_UITagButton[];
extern const uintptr_t base_UITagButton[];
extern const uintptr_t locator_UITagListBox[];
extern const uintptr_t hierarchy_UITagListBox[];
extern const uintptr_t bases_UITagListBox[];
extern const uintptr_t base_UITagListBox[];
extern const uintptr_t locator_UIViewCameraInfoWnd[];
extern const uintptr_t hierarchy_UIViewCameraInfoWnd[];
extern const uintptr_t bases_UIViewCameraInfoWnd[];
extern const uintptr_t base_UIViewCameraInfoWnd[];
extern const uintptr_t locator_UIWindow[];
extern const uintptr_t locator_UICashShopWnd[];
extern const uintptr_t hierarchy_UICashShopWnd[];
extern const uintptr_t bases_UICashShopWnd[];
extern const uintptr_t base_UICashShopWnd[];
extern const uintptr_t locator_UICashShopWnd_Banner[];
extern const uintptr_t hierarchy_UICashShopWnd_Banner[];
extern const uintptr_t bases_UICashShopWnd_Banner[];
extern const uintptr_t base_UICashShopWnd_Banner[];
extern const uintptr_t locator_UICashShopWnd_CartItem[];
extern const uintptr_t hierarchy_UICashShopWnd_CartItem[];
extern const uintptr_t bases_UICashShopWnd_CartItem[];
extern const uintptr_t base_UICashShopWnd_CartItem[];
extern const uintptr_t locator_UICashShopWnd_Cart[];
extern const uintptr_t hierarchy_UICashShopWnd_Cart[];
extern const uintptr_t bases_UICashShopWnd_Cart[];
extern const uintptr_t base_UICashShopWnd_Cart[];
extern const uintptr_t locator_UICashShopWnd_Item[];
extern const uintptr_t hierarchy_UICashShopWnd_Item[];
extern const uintptr_t bases_UICashShopWnd_Item[];
extern const uintptr_t base_UICashShopWnd_Item[];
extern const uintptr_t locator_UINewSelectCharWnd[];
extern const uintptr_t hierarchy_UINewSelectCharWnd[];
extern const uintptr_t bases_UINewSelectCharWnd[];
extern const uintptr_t base_UINewSelectCharWnd[];
extern const uintptr_t locator_UIHairBitmap[];
extern const uintptr_t hierarchy_UIHairBitmap[];
extern const uintptr_t bases_UIHairBitmap[];
extern const uintptr_t base_UIHairBitmap[];
extern const uintptr_t locator_UIHairStyleButton[];
extern const uintptr_t hierarchy_UIHairStyleButton[];
extern const uintptr_t bases_UIHairStyleButton[];
extern const uintptr_t base_UIHairStyleButton[];
extern const uintptr_t locator_UINewMakeCharWnd[];
extern const uintptr_t hierarchy_UINewMakeCharWnd[];
extern const uintptr_t bases_UINewMakeCharWnd[];
extern const uintptr_t base_UINewMakeCharWnd[];
extern const uintptr_t locator_UICheckAttendanceWnd_Banner[];
extern const uintptr_t hierarchy_UICheckAttendanceWnd_Banner[];
extern const uintptr_t bases_UICheckAttendanceWnd_Banner[];
extern const uintptr_t base_UICheckAttendanceWnd_Banner[];
extern const uintptr_t locator_UICheckAttendanceWnd[];
extern const uintptr_t hierarchy_UICheckAttendanceWnd[];
extern const uintptr_t bases_UICheckAttendanceWnd[];
extern const uintptr_t base_UICheckAttendanceWnd[];
extern const uintptr_t locator_CChoicePackageItemBoxWnd[];
extern const uintptr_t hierarchy_CChoicePackageItemBoxWnd[];
extern const uintptr_t bases_CChoicePackageItemBoxWnd[];
extern const uintptr_t base_CChoicePackageItemBoxWnd[];
extern const uintptr_t locator_UIControlRectWnd[];
extern const uintptr_t hierarchy_UIControlRectWnd[];
extern const uintptr_t bases_UIControlRectWnd[];
extern const uintptr_t base_UIControlRectWnd[];
extern const uintptr_t locator_UIConnectTwitterWnd[];
extern const uintptr_t hierarchy_UIConnectTwitterWnd[];
extern const uintptr_t bases_UIConnectTwitterWnd[];
extern const uintptr_t base_UIConnectTwitterWnd[];
extern const uintptr_t base_UIConnectSnsWnd[];
extern const uintptr_t hierarchy_UIConnectSnsWnd[];
extern const uintptr_t bases_UIConnectSnsWnd[];
extern const uintptr_t locator_UIConnectSnsWnd[];
extern const uintptr_t locator_UITwitterScrollBar[];
extern const uintptr_t hierarchy_UITwitterScrollBar[];
extern const uintptr_t bases_UITwitterScrollBar[];
extern const uintptr_t base_UITwitterScrollBar[];
extern const uintptr_t locator_UITwitterEditCtrl3[];
extern const uintptr_t hierarchy_UITwitterEditCtrl3[];
extern const uintptr_t bases_UITwitterEditCtrl3[];
extern const uintptr_t base_UITwitterEditCtrl3[];
extern const uintptr_t locator_UIGradeEnchant_BtnStart[];
extern const uintptr_t hierarchy_UIGradeEnchant_BtnStart[];
extern const uintptr_t bases_UIGradeEnchant_BtnStart[];
extern const uintptr_t base_UIGradeEnchant_BtnStart[];
extern const uintptr_t locator_UIGradeEnchant_BtnEnhancedUp[];
extern const uintptr_t hierarchy_UIGradeEnchant_BtnEnhancedUp[];
extern const uintptr_t bases_UIGradeEnchant_BtnEnhancedUp[];
extern const uintptr_t base_UIGradeEnchant_BtnEnhancedUp[];
extern const uintptr_t locator_UIGradeEnchant_BtnEnhancedDown[];
extern const uintptr_t hierarchy_UIGradeEnchant_BtnEnhancedDown[];
extern const uintptr_t bases_UIGradeEnchant_BtnEnhancedDown[];
extern const uintptr_t base_UIGradeEnchant_BtnEnhancedDown[];
extern const uintptr_t locator_UIGradeEnchantWnd[];
extern const uintptr_t hierarchy_UIGradeEnchantWnd[];
extern const uintptr_t bases_UIGradeEnchantWnd[];
extern const uintptr_t base_UIGradeEnchantWnd[];
extern const uintptr_t locator_UIGraphicSettingWnd[];
extern const uintptr_t hierarchy_UIGraphicSettingWnd[];
extern const uintptr_t bases_UIGraphicSettingWnd[];
extern const uintptr_t base_UIGraphicSettingWnd[];
extern const uintptr_t locator_UICaptchaRegisterWnd[];
extern const uintptr_t hierarchy_UICaptchaRegisterWnd[];
extern const uintptr_t bases_UICaptchaRegisterWnd[];
extern const uintptr_t base_UICaptchaRegisterWnd[];
extern const uintptr_t locator_UIMacroDetectorWnd[];
extern const uintptr_t hierarchy_UIMacroDetectorWnd[];
extern const uintptr_t bases_UIMacroDetectorWnd[];
extern const uintptr_t base_UIMacroDetectorWnd[];
extern const uintptr_t locator_UICaptchaPreviewWnd[];
extern const uintptr_t hierarchy_UICaptchaPreviewWnd[];
extern const uintptr_t bases_UICaptchaPreviewWnd[];
extern const uintptr_t base_UICaptchaPreviewWnd[];
extern const uintptr_t locator_UIMacroReporterWnd[];
extern const uintptr_t hierarchy_UIMacroReporterWnd[];
extern const uintptr_t bases_UIMacroReporterWnd[];
extern const uintptr_t base_UIMacroReporterWnd[];
extern const uintptr_t locator_UIMacroUserInfoWnd[];
extern const uintptr_t hierarchy_UIMacroUserInfoWnd[];
extern const uintptr_t bases_UIMacroUserInfoWnd[];
extern const uintptr_t base_UIMacroUserInfoWnd[];
extern const uintptr_t locator_UIPreviewEquipWnd[];
extern const uintptr_t hierarchy_UIPreviewEquipWnd[];
extern const uintptr_t bases_UIPreviewEquipWnd[];
extern const uintptr_t base_UIPreviewEquipWnd[];
extern const uintptr_t locator_UIRefining_BtnRefining[];
extern const uintptr_t hierarchy_UIRefining_BtnRefining[];
extern const uintptr_t bases_UIRefining_BtnRefining[];
extern const uintptr_t base_UIRefining_BtnRefining[];
extern const uintptr_t locator_UIRefining_BtnReturn[];
extern const uintptr_t hierarchy_UIRefining_BtnReturn[];
extern const uintptr_t bases_UIRefining_BtnReturn[];
extern const uintptr_t base_UIRefining_BtnReturn[];
extern const uintptr_t locator_UIRefining_BtnAgain[];
extern const uintptr_t hierarchy_UIRefining_BtnAgain[];
extern const uintptr_t bases_UIRefining_BtnAgain[];
extern const uintptr_t base_UIRefining_BtnAgain[];
extern const uintptr_t locator_UIRefining_Notice[];
extern const uintptr_t hierarchy_UIRefining_Notice[];
extern const uintptr_t bases_UIRefining_Notice[];
extern const uintptr_t base_UIRefining_Notice[];
extern const uintptr_t locator_UIRefiningWnd[];
extern const uintptr_t hierarchy_UIRefiningWnd[];
extern const uintptr_t bases_UIRefiningWnd[];
extern const uintptr_t base_UIRefiningWnd[];
extern const uintptr_t locator_UINpcSprWnd[];
extern const uintptr_t hierarchy_UINpcSprWnd[];
extern const uintptr_t bases_UINpcSprWnd[];
extern const uintptr_t base_UINpcSprWnd[];
extern const uintptr_t locator_UIOngoingQuestScrollWnd[];
extern const uintptr_t hierarchy_UIOngoingQuestScrollWnd[];
extern const uintptr_t bases_UIOngoingQuestScrollWnd[];
extern const uintptr_t base_UIOngoingQuestScrollWnd[];
extern const uintptr_t locator_UIRecommededQuestScrollWnd[];
extern const uintptr_t hierarchy_UIRecommededQuestScrollWnd[];
extern const uintptr_t bases_UIRecommededQuestScrollWnd[];
extern const uintptr_t base_UIRecommededQuestScrollWnd[];
extern const uintptr_t locator_UIRenewDetailQuestInfoWnd[];
extern const uintptr_t hierarchy_UIRenewDetailQuestInfoWnd[];
extern const uintptr_t bases_UIRenewDetailQuestInfoWnd[];
extern const uintptr_t base_UIRenewDetailQuestInfoWnd[];
extern const uintptr_t locator_UIRenewQuestScrollBar[];
extern const uintptr_t hierarchy_UIRenewQuestScrollBar[];
extern const uintptr_t bases_UIRenewQuestScrollBar[];
extern const uintptr_t base_UIRenewQuestScrollBar[];
extern const uintptr_t locator_UIRenewQuestWnd[];
extern const uintptr_t hierarchy_UIRenewQuestWnd[];
extern const uintptr_t bases_UIRenewQuestWnd[];
extern const uintptr_t base_UIRenewQuestWnd[];
extern const uintptr_t locator_UIRodexScrollBar[];
extern const uintptr_t hierarchy_UIRodexScrollBar[];
extern const uintptr_t bases_UIRodexScrollBar[];
extern const uintptr_t base_UIRodexScrollBar[];
extern const uintptr_t locator_UIRodexEditCtrl3[];
extern const uintptr_t hierarchy_UIRodexEditCtrl3[];
extern const uintptr_t bases_UIRodexEditCtrl3[];
extern const uintptr_t base_UIRodexEditCtrl3[];
extern const uintptr_t locator_UIRodexComposeWnd[];
extern const uintptr_t hierarchy_UIRodexComposeWnd[];
extern const uintptr_t bases_UIRodexComposeWnd[];
extern const uintptr_t base_UIRodexComposeWnd[];
extern const uintptr_t locator_UIRodexContentsWnd[];
extern const uintptr_t hierarchy_UIRodexContentsWnd[];
extern const uintptr_t bases_UIRodexContentsWnd[];
extern const uintptr_t base_UIRodexContentsWnd[];
extern const uintptr_t locator_UIRodexInboxWnd[];
extern const uintptr_t hierarchy_UIRodexInboxWnd[];
extern const uintptr_t bases_UIRodexInboxWnd[];
extern const uintptr_t base_UIRodexInboxWnd[];
extern const uintptr_t locator_UIRoulletteWnd[];
extern const uintptr_t hierarchy_UIRoulletteWnd[];
extern const uintptr_t bases_UIRoulletteWnd[];
extern const uintptr_t base_UIRoulletteWnd[];
extern const uintptr_t locator_UIRenderTargetWnd[];
extern const uintptr_t hierarchy_UIRenderTargetWnd[];
extern const uintptr_t bases_UIRenderTargetWnd[];
extern const uintptr_t base_UIRenderTargetWnd[];
extern const uintptr_t locator_CIcon[];
extern const uintptr_t hierarchy_CIcon[];
extern const uintptr_t bases_CIcon[];
extern const uintptr_t base_CIcon[];
extern const uintptr_t locator_CBmpObjWnd[];
extern const uintptr_t hierarchy_CBmpObjWnd[];
extern const uintptr_t bases_CBmpObjWnd[];
extern const uintptr_t base_CBmpObjWnd[];
extern const uintptr_t locator_CSignBoardWnd[];
extern const uintptr_t hierarchy_CSignBoardWnd[];
extern const uintptr_t bases_CSignBoardWnd[];
extern const uintptr_t base_CSignBoardWnd[];
extern const uintptr_t locator_C3DCooRendering[];
extern const uintptr_t hierarchy_C3DCooRendering[];
extern const uintptr_t bases_C3DCooRendering[];
extern const uintptr_t base_C3DCooRendering[];
extern const uintptr_t locator_UIDisplayCharacter[];
extern const uintptr_t hierarchy_UIDisplayCharacter[];
extern const uintptr_t bases_UIDisplayCharacter[];
extern const uintptr_t base_UIDisplayCharacter[];
extern const uintptr_t locator_UIStylingShopButton[];
extern const uintptr_t hierarchy_UIStylingShopButton[];
extern const uintptr_t bases_UIStylingShopButton[];
extern const uintptr_t base_UIStylingShopButton[];
extern const uintptr_t locator_UIAccBitmapButton[];
extern const uintptr_t hierarchy_UIAccBitmapButton[];
extern const uintptr_t bases_UIAccBitmapButton[];
extern const uintptr_t base_UIAccBitmapButton[];
extern const uintptr_t locator_UI2ndCostumeCheckBox[];
extern const uintptr_t hierarchy_UI2ndCostumeCheckBox[];
extern const uintptr_t bases_UI2ndCostumeCheckBox[];
extern const uintptr_t base_UI2ndCostumeCheckBox[];
extern const uintptr_t locator_UIShowInfoWnd[];
extern const uintptr_t hierarchy_UIShowInfoWnd[];
extern const uintptr_t bases_UIShowInfoWnd[];
extern const uintptr_t base_UIShowInfoWnd[];
extern const uintptr_t locator_UIStylingshopTabParentWnd[];
extern const uintptr_t hierarchy_UIStylingshopTabParentWnd[];
extern const uintptr_t bases_UIStylingshopTabParentWnd[];
extern const uintptr_t base_UIStylingshopTabParentWnd[];
extern const uintptr_t locator_UIBasicStylingWnd[];
extern const uintptr_t hierarchy_UIBasicStylingWnd[];
extern const uintptr_t bases_UIBasicStylingWnd[];
extern const uintptr_t base_UIBasicStylingWnd[];
extern const uintptr_t locator_UIAccessoryCusWnd[];
extern const uintptr_t hierarchy_UIAccessoryCusWnd[];
extern const uintptr_t bases_UIAccessoryCusWnd[];
extern const uintptr_t base_UIAccessoryCusWnd[];
extern const uintptr_t locator_UIAccessoryShowCase[];
extern const uintptr_t hierarchy_UIAccessoryShowCase[];
extern const uintptr_t bases_UIAccessoryShowCase[];
extern const uintptr_t base_UIAccessoryShowCase[];
extern const uintptr_t locator_UI2ndCostumeWnd[];
extern const uintptr_t hierarchy_UI2ndCostumeWnd[];
extern const uintptr_t bases_UI2ndCostumeWnd[];
extern const uintptr_t base_UI2ndCostumeWnd[];
extern const uintptr_t locator_UISlotOfMyStyleWnd[];
extern const uintptr_t hierarchy_UISlotOfMyStyleWnd[];
extern const uintptr_t bases_UISlotOfMyStyleWnd[];
extern const uintptr_t base_UISlotOfMyStyleWnd[];
extern const uintptr_t locator_UISlotForSelectWnd[];
extern const uintptr_t hierarchy_UISlotForSelectWnd[];
extern const uintptr_t bases_UISlotForSelectWnd[];
extern const uintptr_t base_UISlotForSelectWnd[];
extern const uintptr_t locator_UIMyStyleWnd[];
extern const uintptr_t hierarchy_UIMyStyleWnd[];
extern const uintptr_t bases_UIMyStyleWnd[];
extern const uintptr_t base_UIMyStyleWnd[];
extern const uintptr_t locator_UIStylingShopWnd[];
extern const uintptr_t hierarchy_UIStylingShopWnd[];
extern const uintptr_t bases_UIStylingShopWnd[];
extern const uintptr_t base_UIStylingShopWnd[];
extern const uintptr_t locator_UISwapEquipmentWnd[];
extern const uintptr_t hierarchy_UISwapEquipmentWnd[];
extern const uintptr_t bases_UISwapEquipmentWnd[];
extern const uintptr_t base_UISwapEquipmentWnd[];
extern const uintptr_t locator_UISwapBitmapButton[];
extern const uintptr_t hierarchy_UISwapBitmapButton[];
extern const uintptr_t bases_UISwapBitmapButton[];
extern const uintptr_t base_UISwapBitmapButton[];
extern const uintptr_t locator_LineCutter_UIText[];
extern const uintptr_t hierarchy_LineCutter_UIText[];
extern const uintptr_t bases_LineCutter_UIText[];
extern const uintptr_t base_LineCutter_UIText[];
extern const uintptr_t locator_TagTextViewer_UIScroll[];
extern const uintptr_t hierarchy_TagTextViewer_UIScroll[];
extern const uintptr_t bases_TagTextViewer_UIScroll[];
extern const uintptr_t base_TagTextViewer_UIScroll[];
extern const uintptr_t locator_UITagTextViewer[];
extern const uintptr_t hierarchy_UITagTextViewer[];
extern const uintptr_t bases_UITagTextViewer[];
extern const uintptr_t base_UITagTextViewer[];
extern const uintptr_t locator_UIBarGraphPlayerAP[];
extern const uintptr_t hierarchy_UIBarGraphPlayerAP[];
extern const uintptr_t bases_UIBarGraphPlayerAP[];
extern const uintptr_t base_UIBarGraphPlayerAP[];
extern const uintptr_t locator_UIBasicInfoWnd[];
extern const uintptr_t hierarchy_UIBasicInfoWnd[];
extern const uintptr_t bases_UIBasicInfoWnd[];
extern const uintptr_t base_UIBasicInfoWnd[];
extern const uintptr_t locator_UINewSkillListWnd[];
extern const uintptr_t hierarchy_UINewSkillListWnd[];
extern const uintptr_t bases_UINewSkillListWnd[];
extern const uintptr_t base_UINewSkillListWnd[];
extern const uintptr_t locator_UISkipButton[];
extern const uintptr_t hierarchy_UISkipButton[];
extern const uintptr_t bases_UISkipButton[];
extern const uintptr_t base_UISkipButton[];
extern const uintptr_t locator_UIVideoWnd[];
extern const uintptr_t hierarchy_UIVideoWnd[];
extern const uintptr_t bases_UIVideoWnd[];
extern const uintptr_t base_UIVideoWnd[];
extern const uintptr_t locator_UIEntryQueueHelpWnd[];
extern const uintptr_t hierarchy_UIEntryQueueHelpWnd[];
extern const uintptr_t bases_UIEntryQueueHelpWnd[];
extern const uintptr_t base_UIEntryQueueHelpWnd[];
extern const uintptr_t locator_UIEntryQueueListBox[];
extern const uintptr_t hierarchy_UIEntryQueueListBox[];
extern const uintptr_t bases_UIEntryQueueListBox[];
extern const uintptr_t base_UIEntryQueueListBox[];
extern const uintptr_t locator_UIEntryQueueRequestWnd[];
extern const uintptr_t hierarchy_UIEntryQueueRequestWnd[];
extern const uintptr_t bases_UIEntryQueueRequestWnd[];
extern const uintptr_t base_UIEntryQueueRequestWnd[];
extern const uintptr_t locator_UIEntryQueueStandByWnd[];
extern const uintptr_t hierarchy_UIEntryQueueStandByWnd[];
extern const uintptr_t bases_UIEntryQueueStandByWnd[];
extern const uintptr_t base_UIEntryQueueStandByWnd[];
extern const uintptr_t locator_UIEntryQueueWnd[];
extern const uintptr_t hierarchy_UIEntryQueueWnd[];
extern const uintptr_t bases_UIEntryQueueWnd[];
extern const uintptr_t base_UIEntryQueueWnd[];

const uintptr_t locator_CRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CRes, (uintptr_t)hierarchy_CRes};
const uintptr_t hierarchy_CRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)1, (uintptr_t)bases_CRes};
const uintptr_t bases_CRes[] = {(uintptr_t)base_CRes};
const uintptr_t base_CRes[] = {(uintptr_t)&type_CRes, (uintptr_t)0, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CRes};
const uintptr_t locator_CCharacterNpc[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CCharacterNpc, (uintptr_t)hierarchy_CCharacterNpc};
const uintptr_t hierarchy_CCharacterNpc[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_CCharacterNpc};
const uintptr_t bases_CCharacterNpc[] = {(uintptr_t)base_CCharacterNpc, (uintptr_t)base_CNpc, (uintptr_t)base_CGameActor, (uintptr_t)base_CAbleToMakeEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CCharacterNpc[] = {(uintptr_t)&type_CCharacterNpc, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CCharacterNpc};
const uintptr_t base_CNpc[] = {(uintptr_t)&type_CNpc, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CNpc};
const uintptr_t hierarchy_CNpc[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_CNpc};
const uintptr_t bases_CNpc[] = {(uintptr_t)base_CNpc, (uintptr_t)base_CGameActor, (uintptr_t)base_CAbleToMakeEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CGameActor[] = {(uintptr_t)&type_CGameActor, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CGameActor};
const uintptr_t hierarchy_CGameActor[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_CGameActor};
const uintptr_t bases_CGameActor[] = {(uintptr_t)base_CGameActor, (uintptr_t)base_CAbleToMakeEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CAbleToMakeEffect[] = {(uintptr_t)&type_CAbleToMakeEffect, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CAbleToMakeEffect};
const uintptr_t hierarchy_CAbleToMakeEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_CAbleToMakeEffect};
const uintptr_t bases_CAbleToMakeEffect[] = {(uintptr_t)base_CAbleToMakeEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CRenderObject[] = {(uintptr_t)&type_CRenderObject, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CRenderObject};
const uintptr_t hierarchy_CRenderObject[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CRenderObject};
const uintptr_t bases_CRenderObject[] = {(uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CGameObject[] = {(uintptr_t)&type_CGameObject, (uintptr_t)0, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CGameObject};
const uintptr_t hierarchy_CGameObject[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)1, (uintptr_t)bases_CGameObject};
const uintptr_t bases_CGameObject[] = {(uintptr_t)base_CGameObject};
const uintptr_t base_UIEditCtrl[] = {(uintptr_t)&type_UIEditCtrl, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEditCtrl};
const uintptr_t hierarchy_UIEditCtrl[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIEditCtrl};
const uintptr_t bases_UIEditCtrl[] = {(uintptr_t)base_UIEditCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIWindow[] = {(uintptr_t)&type_UIWindow, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIWindow};
const uintptr_t hierarchy_UIWindow[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_UIWindow};
const uintptr_t bases_UIWindow[] = {(uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRPData[] = {(uintptr_t)&type_UIRPData, (uintptr_t)0, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRPData};
const uintptr_t hierarchy_UIRPData[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)1, (uintptr_t)bases_UIRPData};
const uintptr_t bases_UIRPData[] = {(uintptr_t)base_UIRPData};
const uintptr_t locator_UIBitmapEditCtrl[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBitmapEditCtrl, (uintptr_t)hierarchy_UIBitmapEditCtrl};
const uintptr_t hierarchy_UIBitmapEditCtrl[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIBitmapEditCtrl};
const uintptr_t bases_UIBitmapEditCtrl[] = {(uintptr_t)base_UIBitmapEditCtrl, (uintptr_t)base_UIEditCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBitmapEditCtrl[] = {(uintptr_t)&type_UIBitmapEditCtrl, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBitmapEditCtrl};
const uintptr_t base_UIFrameWnd[] = {(uintptr_t)&type_UIFrameWnd, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIFrameWnd};
const uintptr_t hierarchy_UIFrameWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIFrameWnd};
const uintptr_t bases_UIFrameWnd[] = {(uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t locator_UIListBox_NaviSearch[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIListBox_NaviSearch, (uintptr_t)hierarchy_UIListBox_NaviSearch};
const uintptr_t hierarchy_UIListBox_NaviSearch[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIListBox_NaviSearch};
const uintptr_t bases_UIListBox_NaviSearch[] = {(uintptr_t)base_UIListBox_NaviSearch, (uintptr_t)base_UIListBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIListBox_NaviSearch[] = {(uintptr_t)&type_UIListBox_NaviSearch, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIListBox_NaviSearch};
const uintptr_t base_UIListBox[] = {(uintptr_t)&type_UIListBox, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIListBox};
const uintptr_t hierarchy_UIListBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIListBox};
const uintptr_t bases_UIListBox[] = {(uintptr_t)base_UIListBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t locator_UINavigationroadiconWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINavigationroadiconWnd, (uintptr_t)hierarchy_UINavigationroadiconWnd};
const uintptr_t hierarchy_UINavigationroadiconWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UINavigationroadiconWnd};
const uintptr_t bases_UINavigationroadiconWnd[] = {(uintptr_t)base_UINavigationroadiconWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINavigationroadiconWnd[] = {(uintptr_t)&type_UINavigationroadiconWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINavigationroadiconWnd};
const uintptr_t locator_UINavigationRuideWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINavigationRuideWnd, (uintptr_t)hierarchy_UINavigationRuideWnd};
const uintptr_t hierarchy_UINavigationRuideWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UINavigationRuideWnd};
const uintptr_t bases_UINavigationRuideWnd[] = {(uintptr_t)base_UINavigationRuideWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINavigationRuideWnd[] = {(uintptr_t)&type_UINavigationRuideWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINavigationRuideWnd};
const uintptr_t locator_UIPrivateAirplaneWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIPrivateAirplaneWnd, (uintptr_t)hierarchy_UIPrivateAirplaneWnd};
const uintptr_t hierarchy_UIPrivateAirplaneWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIPrivateAirplaneWnd};
const uintptr_t bases_UIPrivateAirplaneWnd[] = {(uintptr_t)base_UIPrivateAirplaneWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIPrivateAirplaneWnd[] = {(uintptr_t)&type_UIPrivateAirplaneWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIPrivateAirplaneWnd};
const uintptr_t locator_UINavigationV4Wnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINavigationV4Wnd, (uintptr_t)hierarchy_UINavigationV4Wnd};
const uintptr_t hierarchy_UINavigationV4Wnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UINavigationV4Wnd};
const uintptr_t bases_UINavigationV4Wnd[] = {(uintptr_t)base_UINavigationV4Wnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINavigationV4Wnd[] = {(uintptr_t)&type_UINavigationV4Wnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINavigationV4Wnd};
const uintptr_t locator_CRollingCutterRagEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CRollingCutterRagEffect, (uintptr_t)hierarchy_CRollingCutterRagEffect};
const uintptr_t hierarchy_CRollingCutterRagEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_CRollingCutterRagEffect};
const uintptr_t bases_CRollingCutterRagEffect[] = {(uintptr_t)base_CRollingCutterRagEffect, (uintptr_t)base_CRagEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CRollingCutterRagEffect[] = {(uintptr_t)&type_CRollingCutterRagEffect, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CRollingCutterRagEffect};
const uintptr_t base_CRagEffect[] = {(uintptr_t)&type_CRagEffect, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CRagEffect};
const uintptr_t hierarchy_CRagEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_CRagEffect};
const uintptr_t bases_CRagEffect[] = {(uintptr_t)base_CRagEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t locator_UIEditCtrl[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEditCtrl, (uintptr_t)hierarchy_UIEditCtrl};
const uintptr_t locator_UIEditCtrl2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEditCtrl2, (uintptr_t)hierarchy_UIEditCtrl2};
const uintptr_t hierarchy_UIEditCtrl2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIEditCtrl2};
const uintptr_t bases_UIEditCtrl2[] = {(uintptr_t)base_UIEditCtrl2, (uintptr_t)base_UIEditCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEditCtrl2[] = {(uintptr_t)&type_UIEditCtrl2, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEditCtrl2};
const uintptr_t locator_UIEditCtrl4[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEditCtrl4, (uintptr_t)hierarchy_UIEditCtrl4};
const uintptr_t hierarchy_UIEditCtrl4[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIEditCtrl4};
const uintptr_t bases_UIEditCtrl4[] = {(uintptr_t)base_UIEditCtrl4, (uintptr_t)base_UIEditCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEditCtrl4[] = {(uintptr_t)&type_UIEditCtrl4, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEditCtrl4};
const uintptr_t locator_UIEditCtrl5[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEditCtrl5, (uintptr_t)hierarchy_UIEditCtrl5};
const uintptr_t hierarchy_UIEditCtrl5[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIEditCtrl5};
const uintptr_t bases_UIEditCtrl5[] = {(uintptr_t)base_UIEditCtrl5, (uintptr_t)base_UIEditCtrl4, (uintptr_t)base_UIEditCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEditCtrl5[] = {(uintptr_t)&type_UIEditCtrl5, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEditCtrl5};
const uintptr_t locator_UIEditCtrl3[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEditCtrl3, (uintptr_t)hierarchy_UIEditCtrl3};
const uintptr_t hierarchy_UIEditCtrl3[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIEditCtrl3};
const uintptr_t bases_UIEditCtrl3[] = {(uintptr_t)base_UIEditCtrl3, (uintptr_t)base_UIEditCtrl2, (uintptr_t)base_UIEditCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEditCtrl3[] = {(uintptr_t)&type_UIEditCtrl3, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEditCtrl3};
const uintptr_t locator_UIChatEditCtrl[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIChatEditCtrl, (uintptr_t)hierarchy_UIChatEditCtrl};
const uintptr_t hierarchy_UIChatEditCtrl[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIChatEditCtrl};
const uintptr_t bases_UIChatEditCtrl[] = {(uintptr_t)base_UIChatEditCtrl, (uintptr_t)base_UIEditCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIChatEditCtrl[] = {(uintptr_t)&type_UIChatEditCtrl, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIChatEditCtrl};
const uintptr_t locator_UIWhisperEditCtrl[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIWhisperEditCtrl, (uintptr_t)hierarchy_UIWhisperEditCtrl};
const uintptr_t hierarchy_UIWhisperEditCtrl[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIWhisperEditCtrl};
const uintptr_t bases_UIWhisperEditCtrl[] = {(uintptr_t)base_UIWhisperEditCtrl, (uintptr_t)base_UIEditCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIWhisperEditCtrl[] = {(uintptr_t)&type_UIWhisperEditCtrl, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIWhisperEditCtrl};
const uintptr_t locator_UIEditCtrl_OldList[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEditCtrl_OldList, (uintptr_t)hierarchy_UIEditCtrl_OldList};
const uintptr_t hierarchy_UIEditCtrl_OldList[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIEditCtrl_OldList};
const uintptr_t bases_UIEditCtrl_OldList[] = {(uintptr_t)base_UIEditCtrl_OldList, (uintptr_t)base_UIEditCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEditCtrl_OldList[] = {(uintptr_t)&type_UIEditCtrl_OldList, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEditCtrl_OldList};
const uintptr_t locator_UIStaticText[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIStaticText, (uintptr_t)hierarchy_UIStaticText};
const uintptr_t hierarchy_UIStaticText[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIStaticText};
const uintptr_t bases_UIStaticText[] = {(uintptr_t)base_UIStaticText, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIStaticText[] = {(uintptr_t)&type_UIStaticText, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIStaticText};
const uintptr_t locator_UIStaticText3[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIStaticText3, (uintptr_t)hierarchy_UIStaticText3};
const uintptr_t hierarchy_UIStaticText3[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIStaticText3};
const uintptr_t bases_UIStaticText3[] = {(uintptr_t)base_UIStaticText3, (uintptr_t)base_UIStaticText, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIStaticText3[] = {(uintptr_t)&type_UIStaticText3, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIStaticText3};
const uintptr_t locator_UIBalloonText[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBalloonText, (uintptr_t)hierarchy_UIBalloonText};
const uintptr_t hierarchy_UIBalloonText[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIBalloonText};
const uintptr_t bases_UIBalloonText[] = {(uintptr_t)base_UIBalloonText, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBalloonText[] = {(uintptr_t)&type_UIBalloonText, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBalloonText};
const uintptr_t locator_UINameBalloonText[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINameBalloonText, (uintptr_t)hierarchy_UINameBalloonText};
const uintptr_t hierarchy_UINameBalloonText[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UINameBalloonText};
const uintptr_t bases_UINameBalloonText[] = {(uintptr_t)base_UINameBalloonText, (uintptr_t)base_UIBalloonText, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINameBalloonText[] = {(uintptr_t)&type_UINameBalloonText, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINameBalloonText};
const uintptr_t locator_UITransBalloonText[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UITransBalloonText, (uintptr_t)hierarchy_UITransBalloonText};
const uintptr_t hierarchy_UITransBalloonText[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UITransBalloonText};
const uintptr_t bases_UITransBalloonText[] = {(uintptr_t)base_UITransBalloonText, (uintptr_t)base_UIBalloonText, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UITransBalloonText[] = {(uintptr_t)&type_UITransBalloonText, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UITransBalloonText};
const uintptr_t locator_UIAchBalloonText[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIAchBalloonText, (uintptr_t)hierarchy_UIAchBalloonText};
const uintptr_t hierarchy_UIAchBalloonText[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIAchBalloonText};
const uintptr_t bases_UIAchBalloonText[] = {(uintptr_t)base_UIAchBalloonText, (uintptr_t)base_UITransBalloonText, (uintptr_t)base_UIBalloonText, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIAchBalloonText[] = {(uintptr_t)&type_UIAchBalloonText, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIAchBalloonText};
const uintptr_t locator_UIButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIButton, (uintptr_t)hierarchy_UIButton};
const uintptr_t hierarchy_UIButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIButton};
const uintptr_t bases_UIButton[] = {(uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIButton[] = {(uintptr_t)&type_UIButton, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIButton};
const uintptr_t locator_UINoneVisibleButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINoneVisibleButton, (uintptr_t)hierarchy_UINoneVisibleButton};
const uintptr_t hierarchy_UINoneVisibleButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UINoneVisibleButton};
const uintptr_t bases_UINoneVisibleButton[] = {(uintptr_t)base_UINoneVisibleButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINoneVisibleButton[] = {(uintptr_t)&type_UINoneVisibleButton, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINoneVisibleButton};
const uintptr_t locator_UIColorButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIColorButton, (uintptr_t)hierarchy_UIColorButton};
const uintptr_t hierarchy_UIColorButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIColorButton};
const uintptr_t bases_UIColorButton[] = {(uintptr_t)base_UIColorButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIColorButton[] = {(uintptr_t)&type_UIColorButton, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIColorButton};
const uintptr_t locator_UIBitmapButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBitmapButton, (uintptr_t)hierarchy_UIBitmapButton};
const uintptr_t hierarchy_UIBitmapButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIBitmapButton};
const uintptr_t bases_UIBitmapButton[] = {(uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBitmapButton[] = {(uintptr_t)&type_UIBitmapButton, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBitmapButton};
const uintptr_t locator_UIBasicButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBasicButton, (uintptr_t)hierarchy_UIBasicButton};
const uintptr_t hierarchy_UIBasicButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIBasicButton};
const uintptr_t bases_UIBasicButton[] = {(uintptr_t)base_UIBasicButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBasicButton[] = {(uintptr_t)&type_UIBasicButton, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBasicButton};
const uintptr_t locator_UIBmpTextButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBmpTextButton, (uintptr_t)hierarchy_UIBmpTextButton};
const uintptr_t hierarchy_UIBmpTextButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIBmpTextButton};
const uintptr_t bases_UIBmpTextButton[] = {(uintptr_t)base_UIBmpTextButton, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBmpTextButton[] = {(uintptr_t)&type_UIBmpTextButton, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBmpTextButton};
const uintptr_t locator_UIBmpTextButton2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBmpTextButton2, (uintptr_t)hierarchy_UIBmpTextButton2};
const uintptr_t hierarchy_UIBmpTextButton2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIBmpTextButton2};
const uintptr_t bases_UIBmpTextButton2[] = {(uintptr_t)base_UIBmpTextButton2, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBmpTextButton2[] = {(uintptr_t)&type_UIBmpTextButton2, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBmpTextButton2};
const uintptr_t locator_UITextureButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UITextureButton, (uintptr_t)hierarchy_UITextureButton};
const uintptr_t hierarchy_UITextureButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UITextureButton};
const uintptr_t bases_UITextureButton[] = {(uintptr_t)base_UITextureButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UITextureButton[] = {(uintptr_t)&type_UITextureButton, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UITextureButton};
const uintptr_t locator_UISpriteButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISpriteButton, (uintptr_t)hierarchy_UISpriteButton};
const uintptr_t hierarchy_UISpriteButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UISpriteButton};
const uintptr_t bases_UISpriteButton[] = {(uintptr_t)base_UISpriteButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISpriteButton[] = {(uintptr_t)&type_UISpriteButton, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISpriteButton};
const uintptr_t locator_UIHeadsprButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIHeadsprButton, (uintptr_t)hierarchy_UIHeadsprButton};
const uintptr_t hierarchy_UIHeadsprButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIHeadsprButton};
const uintptr_t bases_UIHeadsprButton[] = {(uintptr_t)base_UIHeadsprButton, (uintptr_t)base_UISpriteButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIHeadsprButton[] = {(uintptr_t)&type_UIHeadsprButton, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIHeadsprButton};
const uintptr_t locator_UICheckBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UICheckBox, (uintptr_t)hierarchy_UICheckBox};
const uintptr_t hierarchy_UICheckBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UICheckBox};
const uintptr_t bases_UICheckBox[] = {(uintptr_t)base_UICheckBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UICheckBox[] = {(uintptr_t)&type_UICheckBox, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UICheckBox};
const uintptr_t locator_UIStaticBitmap[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIStaticBitmap, (uintptr_t)hierarchy_UIStaticBitmap};
const uintptr_t hierarchy_UIStaticBitmap[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIStaticBitmap};
const uintptr_t bases_UIStaticBitmap[] = {(uintptr_t)base_UIStaticBitmap, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIStaticBitmap[] = {(uintptr_t)&type_UIStaticBitmap, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIStaticBitmap};
const uintptr_t locator_UISlotBitmap[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISlotBitmap, (uintptr_t)hierarchy_UISlotBitmap};
const uintptr_t hierarchy_UISlotBitmap[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UISlotBitmap};
const uintptr_t bases_UISlotBitmap[] = {(uintptr_t)base_UISlotBitmap, (uintptr_t)base_UIStaticBitmap, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISlotBitmap[] = {(uintptr_t)&type_UISlotBitmap, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISlotBitmap};
const uintptr_t locator_UIRadioBtn[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRadioBtn, (uintptr_t)hierarchy_UIRadioBtn};
const uintptr_t hierarchy_UIRadioBtn[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIRadioBtn};
const uintptr_t bases_UIRadioBtn[] = {(uintptr_t)base_UIRadioBtn, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRadioBtn[] = {(uintptr_t)&type_UIRadioBtn, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRadioBtn};
const uintptr_t locator_UIResizer[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIResizer, (uintptr_t)hierarchy_UIResizer};
const uintptr_t hierarchy_UIResizer[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIResizer};
const uintptr_t bases_UIResizer[] = {(uintptr_t)base_UIResizer, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIResizer[] = {(uintptr_t)&type_UIResizer, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIResizer};
const uintptr_t locator_UITab[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UITab, (uintptr_t)hierarchy_UITab};
const uintptr_t hierarchy_UITab[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UITab};
const uintptr_t bases_UITab[] = {(uintptr_t)base_UITab, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UITab[] = {(uintptr_t)&type_UITab, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UITab};
const uintptr_t locator_UIBitmapTab[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBitmapTab, (uintptr_t)hierarchy_UIBitmapTab};
const uintptr_t hierarchy_UIBitmapTab[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIBitmapTab};
const uintptr_t bases_UIBitmapTab[] = {(uintptr_t)base_UIBitmapTab, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBitmapTab[] = {(uintptr_t)&type_UIBitmapTab, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBitmapTab};
const uintptr_t locator_UIChatRoomTitle[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIChatRoomTitle, (uintptr_t)hierarchy_UIChatRoomTitle};
const uintptr_t hierarchy_UIChatRoomTitle[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIChatRoomTitle};
const uintptr_t bases_UIChatRoomTitle[] = {(uintptr_t)base_UIChatRoomTitle, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIChatRoomTitle[] = {(uintptr_t)&type_UIChatRoomTitle, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIChatRoomTitle};
const uintptr_t locator_UIMerchantShopTitle[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMerchantShopTitle, (uintptr_t)hierarchy_UIMerchantShopTitle};
const uintptr_t hierarchy_UIMerchantShopTitle[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIMerchantShopTitle};
const uintptr_t bases_UIMerchantShopTitle[] = {(uintptr_t)base_UIMerchantShopTitle, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMerchantShopTitle[] = {(uintptr_t)&type_UIMerchantShopTitle, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMerchantShopTitle};
const uintptr_t locator_UIEmblemWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEmblemWnd, (uintptr_t)hierarchy_UIEmblemWnd};
const uintptr_t hierarchy_UIEmblemWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIEmblemWnd};
const uintptr_t bases_UIEmblemWnd[] = {(uintptr_t)base_UIEmblemWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEmblemWnd[] = {(uintptr_t)&type_UIEmblemWnd, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEmblemWnd};
const uintptr_t locator_UIWordDisplayWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIWordDisplayWnd, (uintptr_t)hierarchy_UIWordDisplayWnd};
const uintptr_t hierarchy_UIWordDisplayWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIWordDisplayWnd};
const uintptr_t bases_UIWordDisplayWnd[] = {(uintptr_t)base_UIWordDisplayWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIWordDisplayWnd[] = {(uintptr_t)&type_UIWordDisplayWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIWordDisplayWnd};
const uintptr_t locator_UIScrollBar[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIScrollBar, (uintptr_t)hierarchy_UIScrollBar};
const uintptr_t hierarchy_UIScrollBar[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIScrollBar};
const uintptr_t bases_UIScrollBar[] = {(uintptr_t)base_UIScrollBar, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIScrollBar[] = {(uintptr_t)&type_UIScrollBar, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIScrollBar};
const uintptr_t locator_UIScrollBar2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIScrollBar2, (uintptr_t)hierarchy_UIScrollBar2};
const uintptr_t hierarchy_UIScrollBar2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIScrollBar2};
const uintptr_t bases_UIScrollBar2[] = {(uintptr_t)base_UIScrollBar2, (uintptr_t)base_UIScrollBar, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIScrollBar2[] = {(uintptr_t)&type_UIScrollBar2, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIScrollBar2};
const uintptr_t locator_UIScrollBar3[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIScrollBar3, (uintptr_t)hierarchy_UIScrollBar3};
const uintptr_t hierarchy_UIScrollBar3[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIScrollBar3};
const uintptr_t bases_UIScrollBar3[] = {(uintptr_t)base_UIScrollBar3, (uintptr_t)base_UIScrollBar, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIScrollBar3[] = {(uintptr_t)&type_UIScrollBar3, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIScrollBar3};
const uintptr_t locator_UIScrollBar4[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIScrollBar4, (uintptr_t)hierarchy_UIScrollBar4};
const uintptr_t hierarchy_UIScrollBar4[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIScrollBar4};
const uintptr_t bases_UIScrollBar4[] = {(uintptr_t)base_UIScrollBar4, (uintptr_t)base_UIScrollBar, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIScrollBar4[] = {(uintptr_t)&type_UIScrollBar4, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIScrollBar4};
const uintptr_t locator_UIScrollBar_Custom[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIScrollBar_Custom, (uintptr_t)hierarchy_UIScrollBar_Custom};
const uintptr_t hierarchy_UIScrollBar_Custom[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIScrollBar_Custom};
const uintptr_t bases_UIScrollBar_Custom[] = {(uintptr_t)base_UIScrollBar_Custom, (uintptr_t)base_UIScrollBar, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIScrollBar_Custom[] = {(uintptr_t)&type_UIScrollBar_Custom, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIScrollBar_Custom};
const uintptr_t locator_UIZeroQuestScrollBar[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIZeroQuestScrollBar, (uintptr_t)hierarchy_UIZeroQuestScrollBar};
const uintptr_t hierarchy_UIZeroQuestScrollBar[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIZeroQuestScrollBar};
const uintptr_t bases_UIZeroQuestScrollBar[] = {(uintptr_t)base_UIZeroQuestScrollBar, (uintptr_t)base_UIScrollBar, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIZeroQuestScrollBar[] = {(uintptr_t)&type_UIZeroQuestScrollBar, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIZeroQuestScrollBar};
const uintptr_t locator_UITextButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UITextButton, (uintptr_t)hierarchy_UITextButton};
const uintptr_t hierarchy_UITextButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UITextButton};
const uintptr_t bases_UITextButton[] = {(uintptr_t)base_UITextButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UITextButton[] = {(uintptr_t)&type_UITextButton, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UITextButton};
const uintptr_t locator_UIItemTagButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemTagButton, (uintptr_t)hierarchy_UIItemTagButton};
const uintptr_t hierarchy_UIItemTagButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIItemTagButton};
const uintptr_t bases_UIItemTagButton[] = {(uintptr_t)base_UIItemTagButton, (uintptr_t)base_UITextButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemTagButton[] = {(uintptr_t)&type_UIItemTagButton, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemTagButton};
const uintptr_t locator_UIEditCtrlVarId[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEditCtrlVarId, (uintptr_t)hierarchy_UIEditCtrlVarId};
const uintptr_t hierarchy_UIEditCtrlVarId[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIEditCtrlVarId};
const uintptr_t bases_UIEditCtrlVarId[] = {(uintptr_t)base_UIEditCtrlVarId, (uintptr_t)base_UIEditCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEditCtrlVarId[] = {(uintptr_t)&type_UIEditCtrlVarId, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEditCtrlVarId};
const uintptr_t locator_UIItemTagOnChat[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemTagOnChat, (uintptr_t)hierarchy_UIItemTagOnChat};
const uintptr_t hierarchy_UIItemTagOnChat[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIItemTagOnChat};
const uintptr_t bases_UIItemTagOnChat[] = {(uintptr_t)base_UIItemTagOnChat, (uintptr_t)base_UIItemTagCtrl, (uintptr_t)base_UIEditCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemTagOnChat[] = {(uintptr_t)&type_UIItemTagOnChat, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemTagOnChat};
const uintptr_t base_UIItemTagCtrl[] = {(uintptr_t)&type_UIItemTagCtrl, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemTagCtrl};
const uintptr_t hierarchy_UIItemTagCtrl[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIItemTagCtrl};
const uintptr_t bases_UIItemTagCtrl[] = {(uintptr_t)base_UIItemTagCtrl, (uintptr_t)base_UIEditCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t locator_UIBarGraphPlayer[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBarGraphPlayer, (uintptr_t)hierarchy_UIBarGraphPlayer};
const uintptr_t hierarchy_UIBarGraphPlayer[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIBarGraphPlayer};
const uintptr_t bases_UIBarGraphPlayer[] = {(uintptr_t)base_UIBarGraphPlayer, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBarGraphPlayer[] = {(uintptr_t)&type_UIBarGraphPlayer, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBarGraphPlayer};
const uintptr_t locator_UIBarGraphHomun[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBarGraphHomun, (uintptr_t)hierarchy_UIBarGraphHomun};
const uintptr_t hierarchy_UIBarGraphHomun[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIBarGraphHomun};
const uintptr_t bases_UIBarGraphHomun[] = {(uintptr_t)base_UIBarGraphHomun, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBarGraphHomun[] = {(uintptr_t)&type_UIBarGraphHomun, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBarGraphHomun};
const uintptr_t locator_UIBarGraph[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBarGraph, (uintptr_t)hierarchy_UIBarGraph};
const uintptr_t hierarchy_UIBarGraph[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIBarGraph};
const uintptr_t bases_UIBarGraph[] = {(uintptr_t)base_UIBarGraph, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBarGraph[] = {(uintptr_t)&type_UIBarGraph, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBarGraph};
const uintptr_t locator_UIBarGraph2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBarGraph2, (uintptr_t)hierarchy_UIBarGraph2};
const uintptr_t hierarchy_UIBarGraph2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIBarGraph2};
const uintptr_t bases_UIBarGraph2[] = {(uintptr_t)base_UIBarGraph2, (uintptr_t)base_UIBarGraph, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBarGraph2[] = {(uintptr_t)&type_UIBarGraph2, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBarGraph2};
const uintptr_t locator_UIBarGraph3[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBarGraph3, (uintptr_t)hierarchy_UIBarGraph3};
const uintptr_t hierarchy_UIBarGraph3[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIBarGraph3};
const uintptr_t bases_UIBarGraph3[] = {(uintptr_t)base_UIBarGraph3, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBarGraph3[] = {(uintptr_t)&type_UIBarGraph3, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBarGraph3};
const uintptr_t locator_UIINT64BarGraph[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIINT64BarGraph, (uintptr_t)hierarchy_UIINT64BarGraph};
const uintptr_t hierarchy_UIINT64BarGraph[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIINT64BarGraph};
const uintptr_t bases_UIINT64BarGraph[] = {(uintptr_t)base_UIINT64BarGraph, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIINT64BarGraph[] = {(uintptr_t)&type_UIINT64BarGraph, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIINT64BarGraph};
const uintptr_t locator_UIPetInfoBarGraph[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIPetInfoBarGraph, (uintptr_t)hierarchy_UIPetInfoBarGraph};
const uintptr_t hierarchy_UIPetInfoBarGraph[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIPetInfoBarGraph};
const uintptr_t bases_UIPetInfoBarGraph[] = {(uintptr_t)base_UIPetInfoBarGraph, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIPetInfoBarGraph[] = {(uintptr_t)&type_UIPetInfoBarGraph, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIPetInfoBarGraph};
const uintptr_t locator_UIGage[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIGage, (uintptr_t)hierarchy_UIGage};
const uintptr_t hierarchy_UIGage[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIGage};
const uintptr_t bases_UIGage[] = {(uintptr_t)base_UIGage, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIGage[] = {(uintptr_t)&type_UIGage, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIGage};
const uintptr_t locator_UIRechargeGage[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRechargeGage, (uintptr_t)hierarchy_UIRechargeGage};
const uintptr_t hierarchy_UIRechargeGage[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIRechargeGage};
const uintptr_t bases_UIRechargeGage[] = {(uintptr_t)base_UIRechargeGage, (uintptr_t)base_UIGage, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRechargeGage[] = {(uintptr_t)&type_UIRechargeGage, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRechargeGage};
const uintptr_t locator_UIPcGage[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIPcGage, (uintptr_t)hierarchy_UIPcGage};
const uintptr_t hierarchy_UIPcGage[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIPcGage};
const uintptr_t bases_UIPcGage[] = {(uintptr_t)base_UIPcGage, (uintptr_t)base_UIGage, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIPcGage[] = {(uintptr_t)&type_UIPcGage, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIPcGage};
const uintptr_t locator_UIPlayerGage[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIPlayerGage, (uintptr_t)hierarchy_UIPlayerGage};
const uintptr_t hierarchy_UIPlayerGage[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIPlayerGage};
const uintptr_t bases_UIPlayerGage[] = {(uintptr_t)base_UIPlayerGage, (uintptr_t)base_UIGage, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIPlayerGage[] = {(uintptr_t)&type_UIPlayerGage, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIPlayerGage};
const uintptr_t locator_UIMonsterGage[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMonsterGage, (uintptr_t)hierarchy_UIMonsterGage};
const uintptr_t hierarchy_UIMonsterGage[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIMonsterGage};
const uintptr_t bases_UIMonsterGage[] = {(uintptr_t)base_UIMonsterGage, (uintptr_t)base_UIPcGage, (uintptr_t)base_UIGage, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMonsterGage[] = {(uintptr_t)&type_UIMonsterGage, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMonsterGage};
const uintptr_t locator_UIColorTransBalloonText[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIColorTransBalloonText, (uintptr_t)hierarchy_UIColorTransBalloonText};
const uintptr_t hierarchy_UIColorTransBalloonText[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIColorTransBalloonText};
const uintptr_t bases_UIColorTransBalloonText[] = {(uintptr_t)base_UIColorTransBalloonText, (uintptr_t)base_UITransBalloonText, (uintptr_t)base_UIBalloonText, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIColorTransBalloonText[] = {(uintptr_t)&type_UIColorTransBalloonText, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIColorTransBalloonText};
const uintptr_t locator_UIBitmapButton2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBitmapButton2, (uintptr_t)hierarchy_UIBitmapButton2};
const uintptr_t hierarchy_UIBitmapButton2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIBitmapButton2};
const uintptr_t bases_UIBitmapButton2[] = {(uintptr_t)base_UIBitmapButton2, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBitmapButton2[] = {(uintptr_t)&type_UIBitmapButton2, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBitmapButton2};
const uintptr_t locator_UIListBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIListBox, (uintptr_t)hierarchy_UIListBox};
const uintptr_t locator_UIPackageBoxNameList[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIPackageBoxNameList, (uintptr_t)hierarchy_UIPackageBoxNameList};
const uintptr_t hierarchy_UIPackageBoxNameList[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIPackageBoxNameList};
const uintptr_t bases_UIPackageBoxNameList[] = {(uintptr_t)base_UIPackageBoxNameList, (uintptr_t)base_UIListBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIPackageBoxNameList[] = {(uintptr_t)&type_UIPackageBoxNameList, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIPackageBoxNameList};
const uintptr_t locator_UIListBox2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIListBox2, (uintptr_t)hierarchy_UIListBox2};
const uintptr_t hierarchy_UIListBox2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIListBox2};
const uintptr_t bases_UIListBox2[] = {(uintptr_t)base_UIListBox2, (uintptr_t)base_UIListBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIListBox2[] = {(uintptr_t)&type_UIListBox2, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIListBox2};
const uintptr_t locator_UIListBoxID[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIListBoxID, (uintptr_t)hierarchy_UIListBoxID};
const uintptr_t hierarchy_UIListBoxID[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIListBoxID};
const uintptr_t bases_UIListBoxID[] = {(uintptr_t)base_UIListBoxID, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIListBoxID[] = {(uintptr_t)&type_UIListBoxID, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIListBoxID};
const uintptr_t locator_UIListBoxNavi[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIListBoxNavi, (uintptr_t)hierarchy_UIListBoxNavi};
const uintptr_t hierarchy_UIListBoxNavi[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIListBoxNavi};
const uintptr_t bases_UIListBoxNavi[] = {(uintptr_t)base_UIListBoxNavi, (uintptr_t)base_UIListBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIListBoxNavi[] = {(uintptr_t)&type_UIListBoxNavi, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIListBoxNavi};
const uintptr_t locator_UIListBoxEx[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIListBoxEx, (uintptr_t)hierarchy_UIListBoxEx};
const uintptr_t hierarchy_UIListBoxEx[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIListBoxEx};
const uintptr_t bases_UIListBoxEx[] = {(uintptr_t)base_UIListBoxEx, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIListBoxEx[] = {(uintptr_t)&type_UIListBoxEx, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIListBoxEx};
const uintptr_t locator_UIListBoxTitle[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIListBoxTitle, (uintptr_t)hierarchy_UIListBoxTitle};
const uintptr_t hierarchy_UIListBoxTitle[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIListBoxTitle};
const uintptr_t bases_UIListBoxTitle[] = {(uintptr_t)base_UIListBoxTitle, (uintptr_t)base_UIListBoxID, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIListBoxTitle[] = {(uintptr_t)&type_UIListBoxTitle, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIListBoxTitle};
const uintptr_t locator_UIComboBoxEx[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIComboBoxEx, (uintptr_t)hierarchy_UIComboBoxEx};
const uintptr_t hierarchy_UIComboBoxEx[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIComboBoxEx};
const uintptr_t bases_UIComboBoxEx[] = {(uintptr_t)base_UIComboBoxEx, (uintptr_t)base_UIListBoxEx, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIComboBoxEx[] = {(uintptr_t)&type_UIComboBoxEx, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIComboBoxEx};
const uintptr_t locator_UIComboBoxEx2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIComboBoxEx2, (uintptr_t)hierarchy_UIComboBoxEx2};
const uintptr_t hierarchy_UIComboBoxEx2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIComboBoxEx2};
const uintptr_t bases_UIComboBoxEx2[] = {(uintptr_t)base_UIComboBoxEx2, (uintptr_t)base_UIComboBoxEx, (uintptr_t)base_UIListBoxEx, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIComboBoxEx2[] = {(uintptr_t)&type_UIComboBoxEx2, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIComboBoxEx2};
const uintptr_t locator_UIItemStoreComboBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemStoreComboBox, (uintptr_t)hierarchy_UIItemStoreComboBox};
const uintptr_t hierarchy_UIItemStoreComboBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIItemStoreComboBox};
const uintptr_t bases_UIItemStoreComboBox[] = {(uintptr_t)base_UIItemStoreComboBox, (uintptr_t)base_UIComboBoxEx, (uintptr_t)base_UIListBoxEx, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemStoreComboBox[] = {(uintptr_t)&type_UIItemStoreComboBox, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemStoreComboBox};
const uintptr_t locator_UIComboBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIComboBox, (uintptr_t)hierarchy_UIComboBox};
const uintptr_t hierarchy_UIComboBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIComboBox};
const uintptr_t bases_UIComboBox[] = {(uintptr_t)base_UIComboBox, (uintptr_t)base_UIListBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIComboBox[] = {(uintptr_t)&type_UIComboBox, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIComboBox};
const uintptr_t locator_UIComboBoxID[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIComboBoxID, (uintptr_t)hierarchy_UIComboBoxID};
const uintptr_t hierarchy_UIComboBoxID[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIComboBoxID};
const uintptr_t bases_UIComboBoxID[] = {(uintptr_t)base_UIComboBoxID, (uintptr_t)base_UIListBoxID, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIComboBoxID[] = {(uintptr_t)&type_UIComboBoxID, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIComboBoxID};
const uintptr_t locator_UIComboBoxDefWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIComboBoxDefWnd, (uintptr_t)hierarchy_UIComboBoxDefWnd};
const uintptr_t hierarchy_UIComboBoxDefWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIComboBoxDefWnd};
const uintptr_t bases_UIComboBoxDefWnd[] = {(uintptr_t)base_UIComboBoxDefWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIComboBoxDefWnd[] = {(uintptr_t)&type_UIComboBoxDefWnd, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIComboBoxDefWnd};
const uintptr_t locator_UIComboBoxDef2Wnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIComboBoxDef2Wnd, (uintptr_t)hierarchy_UIComboBoxDef2Wnd};
const uintptr_t hierarchy_UIComboBoxDef2Wnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIComboBoxDef2Wnd};
const uintptr_t bases_UIComboBoxDef2Wnd[] = {(uintptr_t)base_UIComboBoxDef2Wnd, (uintptr_t)base_UIComboBoxDefWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIComboBoxDef2Wnd[] = {(uintptr_t)&type_UIComboBoxDef2Wnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIComboBoxDef2Wnd};
const uintptr_t locator_UIItemStoreComboBoxWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemStoreComboBoxWnd, (uintptr_t)hierarchy_UIItemStoreComboBoxWnd};
const uintptr_t hierarchy_UIItemStoreComboBoxWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIItemStoreComboBoxWnd};
const uintptr_t bases_UIItemStoreComboBoxWnd[] = {(uintptr_t)base_UIItemStoreComboBoxWnd, (uintptr_t)base_UIComboBoxDefWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemStoreComboBoxWnd[] = {(uintptr_t)&type_UIItemStoreComboBoxWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemStoreComboBoxWnd};
const uintptr_t locator_UIComboBox_BmpWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIComboBox_BmpWnd, (uintptr_t)hierarchy_UIComboBox_BmpWnd};
const uintptr_t hierarchy_UIComboBox_BmpWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIComboBox_BmpWnd};
const uintptr_t bases_UIComboBox_BmpWnd[] = {(uintptr_t)base_UIComboBox_BmpWnd, (uintptr_t)base_UIComboBoxDefWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIComboBox_BmpWnd[] = {(uintptr_t)&type_UIComboBox_BmpWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIComboBox_BmpWnd};
const uintptr_t locator_UIComboBoxV2Wnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIComboBoxV2Wnd, (uintptr_t)hierarchy_UIComboBoxV2Wnd};
const uintptr_t hierarchy_UIComboBoxV2Wnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIComboBoxV2Wnd};
const uintptr_t bases_UIComboBoxV2Wnd[] = {(uintptr_t)base_UIComboBoxV2Wnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIComboBoxV2Wnd[] = {(uintptr_t)&type_UIComboBoxV2Wnd, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIComboBoxV2Wnd};
const uintptr_t locator_TableBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_TableBox, (uintptr_t)hierarchy_TableBox};
const uintptr_t hierarchy_TableBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_TableBox};
const uintptr_t bases_TableBox[] = {(uintptr_t)base_TableBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_TableBox[] = {(uintptr_t)&type_TableBox, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_TableBox};
const uintptr_t locator_UITableBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UITableBox, (uintptr_t)hierarchy_UITableBox};
const uintptr_t hierarchy_UITableBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UITableBox};
const uintptr_t bases_UITableBox[] = {(uintptr_t)base_UITableBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UITableBox[] = {(uintptr_t)&type_UITableBox, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UITableBox};
const uintptr_t locator_UIChatHisBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIChatHisBox, (uintptr_t)hierarchy_UIChatHisBox};
const uintptr_t hierarchy_UIChatHisBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIChatHisBox};
const uintptr_t bases_UIChatHisBox[] = {(uintptr_t)base_UIChatHisBox, (uintptr_t)base_UIListBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIChatHisBox[] = {(uintptr_t)&type_UIChatHisBox, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIChatHisBox};
const uintptr_t locator_UIChatHisBox2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIChatHisBox2, (uintptr_t)hierarchy_UIChatHisBox2};
const uintptr_t hierarchy_UIChatHisBox2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIChatHisBox2};
const uintptr_t bases_UIChatHisBox2[] = {(uintptr_t)base_UIChatHisBox2, (uintptr_t)base_UIChatHisBox, (uintptr_t)base_UIListBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIChatHisBox2[] = {(uintptr_t)&type_UIChatHisBox2, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIChatHisBox2};
const uintptr_t locator_UIChatHisBox3[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIChatHisBox3, (uintptr_t)hierarchy_UIChatHisBox3};
const uintptr_t hierarchy_UIChatHisBox3[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_UIChatHisBox3};
const uintptr_t bases_UIChatHisBox3[] = {(uintptr_t)base_UIChatHisBox3, (uintptr_t)base_UIChatHisBox2, (uintptr_t)base_UIChatHisBox, (uintptr_t)base_UIListBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIChatHisBox3[] = {(uintptr_t)&type_UIChatHisBox3, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIChatHisBox3};
const uintptr_t locator_UIMemberListBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMemberListBox, (uintptr_t)hierarchy_UIMemberListBox};
const uintptr_t hierarchy_UIMemberListBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_UIMemberListBox};
const uintptr_t bases_UIMemberListBox[] = {(uintptr_t)base_UIMemberListBox, (uintptr_t)base_UIChatHisBox2, (uintptr_t)base_UIChatHisBox, (uintptr_t)base_UIListBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMemberListBox[] = {(uintptr_t)&type_UIMemberListBox, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMemberListBox};
const uintptr_t locator_UITextViewer[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UITextViewer, (uintptr_t)hierarchy_UITextViewer};
const uintptr_t hierarchy_UITextViewer[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UITextViewer};
const uintptr_t bases_UITextViewer[] = {(uintptr_t)base_UITextViewer, (uintptr_t)base_UIListBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UITextViewer[] = {(uintptr_t)&type_UITextViewer, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UITextViewer};
const uintptr_t locator_UIString[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIString, (uintptr_t)hierarchy_UIString};
const uintptr_t hierarchy_UIString[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIString};
const uintptr_t bases_UIString[] = {(uintptr_t)base_UIString, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIString[] = {(uintptr_t)&type_UIString, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIString};
const uintptr_t locator_UITextViewEX[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UITextViewEX, (uintptr_t)hierarchy_UITextViewEX};
const uintptr_t hierarchy_UITextViewEX[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UITextViewEX};
const uintptr_t bases_UITextViewEX[] = {(uintptr_t)base_UITextViewEX, (uintptr_t)base_UITextViewer, (uintptr_t)base_UIListBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UITextViewEX[] = {(uintptr_t)&type_UITextViewEX, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UITextViewEX};
const uintptr_t locator_UIServerListBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIServerListBox, (uintptr_t)hierarchy_UIServerListBox};
const uintptr_t hierarchy_UIServerListBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIServerListBox};
const uintptr_t bases_UIServerListBox[] = {(uintptr_t)base_UIServerListBox, (uintptr_t)base_UIListBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIServerListBox[] = {(uintptr_t)&type_UIServerListBox, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIServerListBox};
const uintptr_t locator_UIImgMenu[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIImgMenu, (uintptr_t)hierarchy_UIImgMenu};
const uintptr_t hierarchy_UIImgMenu[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIImgMenu};
const uintptr_t bases_UIImgMenu[] = {(uintptr_t)base_UIImgMenu, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIImgMenu[] = {(uintptr_t)&type_UIImgMenu, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIImgMenu};
const uintptr_t locator_UITVTextButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UITVTextButton, (uintptr_t)hierarchy_UITVTextButton};
const uintptr_t hierarchy_UITVTextButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UITVTextButton};
const uintptr_t bases_UITVTextButton[] = {(uintptr_t)base_UITVTextButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UITVTextButton[] = {(uintptr_t)&type_UITVTextButton, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UITVTextButton};
const uintptr_t locator_UITVBitmapButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UITVBitmapButton, (uintptr_t)hierarchy_UITVBitmapButton};
const uintptr_t hierarchy_UITVBitmapButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UITVBitmapButton};
const uintptr_t bases_UITVBitmapButton[] = {(uintptr_t)base_UITVBitmapButton, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UITVBitmapButton[] = {(uintptr_t)&type_UITVBitmapButton, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UITVBitmapButton};
const uintptr_t locator_UITreeView[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UITreeView, (uintptr_t)hierarchy_UITreeView};
const uintptr_t hierarchy_UITreeView[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UITreeView};
const uintptr_t bases_UITreeView[] = {(uintptr_t)base_UITreeView, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UITreeView[] = {(uintptr_t)&type_UITreeView, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UITreeView};
const uintptr_t locator_UITabControl[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UITabControl, (uintptr_t)hierarchy_UITabControl};
const uintptr_t hierarchy_UITabControl[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UITabControl};
const uintptr_t bases_UITabControl[] = {(uintptr_t)base_UITabControl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UITabControl[] = {(uintptr_t)&type_UITabControl, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UITabControl};
const uintptr_t locator_UIShowAndEditTimeCtrl[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIShowAndEditTimeCtrl, (uintptr_t)hierarchy_UIShowAndEditTimeCtrl};
const uintptr_t hierarchy_UIShowAndEditTimeCtrl[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIShowAndEditTimeCtrl};
const uintptr_t bases_UIShowAndEditTimeCtrl[] = {(uintptr_t)base_UIShowAndEditTimeCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIShowAndEditTimeCtrl[] = {(uintptr_t)&type_UIShowAndEditTimeCtrl, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIShowAndEditTimeCtrl};
const uintptr_t locator_UIBasicItemCtrl[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBasicItemCtrl, (uintptr_t)hierarchy_UIBasicItemCtrl};
const uintptr_t hierarchy_UIBasicItemCtrl[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIBasicItemCtrl};
const uintptr_t bases_UIBasicItemCtrl[] = {(uintptr_t)base_UIBasicItemCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBasicItemCtrl[] = {(uintptr_t)&type_UIBasicItemCtrl, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBasicItemCtrl};
const uintptr_t locator_UIBasicSkillCtrl[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBasicSkillCtrl, (uintptr_t)hierarchy_UIBasicSkillCtrl};
const uintptr_t hierarchy_UIBasicSkillCtrl[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIBasicSkillCtrl};
const uintptr_t bases_UIBasicSkillCtrl[] = {(uintptr_t)base_UIBasicSkillCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBasicSkillCtrl[] = {(uintptr_t)&type_UIBasicSkillCtrl, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBasicSkillCtrl};
const uintptr_t locator_UINewComboBoxCtrl[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINewComboBoxCtrl, (uintptr_t)hierarchy_UINewComboBoxCtrl};
const uintptr_t hierarchy_UINewComboBoxCtrl[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UINewComboBoxCtrl};
const uintptr_t bases_UINewComboBoxCtrl[] = {(uintptr_t)base_UINewComboBoxCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINewComboBoxCtrl[] = {(uintptr_t)&type_UINewComboBoxCtrl, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINewComboBoxCtrl};
const uintptr_t locator_UIItemMoveInfo[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemMoveInfo, (uintptr_t)hierarchy_UIItemMoveInfo};
const uintptr_t hierarchy_UIItemMoveInfo[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIItemMoveInfo};
const uintptr_t bases_UIItemMoveInfo[] = {(uintptr_t)base_UIItemMoveInfo, (uintptr_t)base_UITextButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemMoveInfo[] = {(uintptr_t)&type_UIItemMoveInfo, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemMoveInfo};
const uintptr_t locator_UIItemTagCtrl[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemTagCtrl, (uintptr_t)hierarchy_UIItemTagCtrl};
const uintptr_t locator_UIItemTagOnMegaPhone[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemTagOnMegaPhone, (uintptr_t)hierarchy_UIItemTagOnMegaPhone};
const uintptr_t hierarchy_UIItemTagOnMegaPhone[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIItemTagOnMegaPhone};
const uintptr_t bases_UIItemTagOnMegaPhone[] = {(uintptr_t)base_UIItemTagOnMegaPhone, (uintptr_t)base_UIItemTagCtrl, (uintptr_t)base_UIEditCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemTagOnMegaPhone[] = {(uintptr_t)&type_UIItemTagOnMegaPhone, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemTagOnMegaPhone};
const uintptr_t locator_UIItemTagOnMail[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemTagOnMail, (uintptr_t)hierarchy_UIItemTagOnMail};
const uintptr_t hierarchy_UIItemTagOnMail[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIItemTagOnMail};
const uintptr_t bases_UIItemTagOnMail[] = {(uintptr_t)base_UIItemTagOnMail, (uintptr_t)base_UIItemTagCtrl, (uintptr_t)base_UIEditCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemTagOnMail[] = {(uintptr_t)&type_UIItemTagOnMail, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemTagOnMail};
const uintptr_t locator_UIBmpRadioBtn[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBmpRadioBtn, (uintptr_t)hierarchy_UIBmpRadioBtn};
const uintptr_t hierarchy_UIBmpRadioBtn[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIBmpRadioBtn};
const uintptr_t bases_UIBmpRadioBtn[] = {(uintptr_t)base_UIBmpRadioBtn, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBmpRadioBtn[] = {(uintptr_t)&type_UIBmpRadioBtn, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBmpRadioBtn};
const uintptr_t locator_UIEditCtrl_Custom[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEditCtrl_Custom, (uintptr_t)hierarchy_UIEditCtrl_Custom};
const uintptr_t hierarchy_UIEditCtrl_Custom[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIEditCtrl_Custom};
const uintptr_t bases_UIEditCtrl_Custom[] = {(uintptr_t)base_UIEditCtrl_Custom, (uintptr_t)base_UIEditCtrl2, (uintptr_t)base_UIEditCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEditCtrl_Custom[] = {(uintptr_t)&type_UIEditCtrl_Custom, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEditCtrl_Custom};
const uintptr_t locator_UIEditCtrl_Currency[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEditCtrl_Currency, (uintptr_t)hierarchy_UIEditCtrl_Currency};
const uintptr_t hierarchy_UIEditCtrl_Currency[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_UIEditCtrl_Currency};
const uintptr_t bases_UIEditCtrl_Currency[] = {(uintptr_t)base_UIEditCtrl_Currency, (uintptr_t)base_UIEditCtrl_Custom, (uintptr_t)base_UIEditCtrl2, (uintptr_t)base_UIEditCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEditCtrl_Currency[] = {(uintptr_t)&type_UIEditCtrl_Currency, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEditCtrl_Currency};
const uintptr_t locator_UICaptchaImage[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UICaptchaImage, (uintptr_t)hierarchy_UICaptchaImage};
const uintptr_t hierarchy_UICaptchaImage[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UICaptchaImage};
const uintptr_t bases_UICaptchaImage[] = {(uintptr_t)base_UICaptchaImage, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UICaptchaImage[] = {(uintptr_t)&type_UICaptchaImage, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UICaptchaImage};
const uintptr_t locator_UIStaticItem[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIStaticItem, (uintptr_t)hierarchy_UIStaticItem};
const uintptr_t hierarchy_UIStaticItem[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIStaticItem};
const uintptr_t bases_UIStaticItem[] = {(uintptr_t)base_UIStaticItem, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIStaticItem[] = {(uintptr_t)&type_UIStaticItem, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIStaticItem};
const uintptr_t locator_UIStaticItem_Illust[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIStaticItem_Illust, (uintptr_t)hierarchy_UIStaticItem_Illust};
const uintptr_t hierarchy_UIStaticItem_Illust[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIStaticItem_Illust};
const uintptr_t bases_UIStaticItem_Illust[] = {(uintptr_t)base_UIStaticItem_Illust, (uintptr_t)base_UIStaticItem, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIStaticItem_Illust[] = {(uintptr_t)&type_UIStaticItem_Illust, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIStaticItem_Illust};
const uintptr_t locator_UINaviLinkButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINaviLinkButton, (uintptr_t)hierarchy_UINaviLinkButton};
const uintptr_t hierarchy_UINaviLinkButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UINaviLinkButton};
const uintptr_t bases_UINaviLinkButton[] = {(uintptr_t)base_UINaviLinkButton, (uintptr_t)base_UITextButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINaviLinkButton[] = {(uintptr_t)&type_UINaviLinkButton, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINaviLinkButton};
const uintptr_t locator_UISlotBmpButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISlotBmpButton, (uintptr_t)hierarchy_UISlotBmpButton};
const uintptr_t hierarchy_UISlotBmpButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UISlotBmpButton};
const uintptr_t bases_UISlotBmpButton[] = {(uintptr_t)base_UISlotBmpButton, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISlotBmpButton[] = {(uintptr_t)&type_UISlotBmpButton, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISlotBmpButton};
const uintptr_t locator_UIDescriptRace[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIDescriptRace, (uintptr_t)hierarchy_UIDescriptRace};
const uintptr_t hierarchy_UIDescriptRace[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIDescriptRace};
const uintptr_t bases_UIDescriptRace[] = {(uintptr_t)base_UIDescriptRace, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIDescriptRace[] = {(uintptr_t)&type_UIDescriptRace, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIDescriptRace};
const uintptr_t locator_UIRPImageWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRPImageWnd, (uintptr_t)hierarchy_UIRPImageWnd};
const uintptr_t hierarchy_UIRPImageWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIRPImageWnd};
const uintptr_t bases_UIRPImageWnd[] = {(uintptr_t)base_UIRPImageWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRPImageWnd[] = {(uintptr_t)&type_UIRPImageWnd, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRPImageWnd};
const uintptr_t locator_UIScrollBar_Navi[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIScrollBar_Navi, (uintptr_t)hierarchy_UIScrollBar_Navi};
const uintptr_t hierarchy_UIScrollBar_Navi[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIScrollBar_Navi};
const uintptr_t bases_UIScrollBar_Navi[] = {(uintptr_t)base_UIScrollBar_Navi, (uintptr_t)base_UIScrollBar, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIScrollBar_Navi[] = {(uintptr_t)&type_UIScrollBar_Navi, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIScrollBar_Navi};
const uintptr_t locator_UIWaitWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIWaitWnd, (uintptr_t)hierarchy_UIWaitWnd};
const uintptr_t hierarchy_UIWaitWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIWaitWnd};
const uintptr_t bases_UIWaitWnd[] = {(uintptr_t)base_UIWaitWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIWaitWnd[] = {(uintptr_t)&type_UIWaitWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIWaitWnd};
const uintptr_t locator_UIExchangeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIExchangeWnd, (uintptr_t)hierarchy_UIExchangeWnd};
const uintptr_t hierarchy_UIExchangeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIExchangeWnd};
const uintptr_t bases_UIExchangeWnd[] = {(uintptr_t)base_UIExchangeWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIExchangeWnd[] = {(uintptr_t)&type_UIExchangeWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIExchangeWnd};
const uintptr_t locator_UILoadingWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UILoadingWnd, (uintptr_t)hierarchy_UILoadingWnd};
const uintptr_t hierarchy_UILoadingWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UILoadingWnd};
const uintptr_t bases_UILoadingWnd[] = {(uintptr_t)base_UILoadingWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UILoadingWnd[] = {(uintptr_t)&type_UILoadingWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UILoadingWnd};
const uintptr_t locator_UIToolTipWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIToolTipWnd, (uintptr_t)hierarchy_UIToolTipWnd};
const uintptr_t hierarchy_UIToolTipWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIToolTipWnd};
const uintptr_t bases_UIToolTipWnd[] = {(uintptr_t)base_UIToolTipWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIToolTipWnd[] = {(uintptr_t)&type_UIToolTipWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIToolTipWnd};
const uintptr_t locator_UIEquipWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEquipWnd, (uintptr_t)hierarchy_UIEquipWnd};
const uintptr_t hierarchy_UIEquipWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIEquipWnd};
const uintptr_t bases_UIEquipWnd[] = {(uintptr_t)base_UIEquipWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEquipWnd[] = {(uintptr_t)&type_UIEquipWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEquipWnd};
const uintptr_t locator_UInCashWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UInCashWnd, (uintptr_t)hierarchy_UInCashWnd};
const uintptr_t hierarchy_UInCashWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UInCashWnd};
const uintptr_t bases_UInCashWnd[] = {(uintptr_t)base_UInCashWnd, (uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UInCashWnd[] = {(uintptr_t)&type_UInCashWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UInCashWnd};
const uintptr_t base_UIItemBaseWnd[] = {(uintptr_t)&type_UIItemBaseWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemBaseWnd};
const uintptr_t hierarchy_UIItemBaseWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIItemBaseWnd};
const uintptr_t bases_UIItemBaseWnd[] = {(uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t locator_UIItemParamChangeDisplayWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemParamChangeDisplayWnd, (uintptr_t)hierarchy_UIItemParamChangeDisplayWnd};
const uintptr_t hierarchy_UIItemParamChangeDisplayWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIItemParamChangeDisplayWnd};
const uintptr_t bases_UIItemParamChangeDisplayWnd[] = {(uintptr_t)base_UIItemParamChangeDisplayWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemParamChangeDisplayWnd[] = {(uintptr_t)&type_UIItemParamChangeDisplayWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemParamChangeDisplayWnd};
const uintptr_t locator_UIPasswordWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIPasswordWnd, (uintptr_t)hierarchy_UIPasswordWnd};
const uintptr_t hierarchy_UIPasswordWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIPasswordWnd};
const uintptr_t bases_UIPasswordWnd[] = {(uintptr_t)base_UIPasswordWnd, (uintptr_t)base_UIItemDropCntWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIPasswordWnd[] = {(uintptr_t)&type_UIPasswordWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIPasswordWnd};
const uintptr_t base_UIItemDropCntWnd[] = {(uintptr_t)&type_UIItemDropCntWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemDropCntWnd};
const uintptr_t hierarchy_UIItemDropCntWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIItemDropCntWnd};
const uintptr_t bases_UIItemDropCntWnd[] = {(uintptr_t)base_UIItemDropCntWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t locator_UIEmailAddressWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEmailAddressWnd, (uintptr_t)hierarchy_UIEmailAddressWnd};
const uintptr_t hierarchy_UIEmailAddressWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIEmailAddressWnd};
const uintptr_t bases_UIEmailAddressWnd[] = {(uintptr_t)base_UIEmailAddressWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEmailAddressWnd[] = {(uintptr_t)&type_UIEmailAddressWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEmailAddressWnd};
const uintptr_t locator_UISkillNameChangeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISkillNameChangeWnd, (uintptr_t)hierarchy_UISkillNameChangeWnd};
const uintptr_t hierarchy_UISkillNameChangeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UISkillNameChangeWnd};
const uintptr_t bases_UISkillNameChangeWnd[] = {(uintptr_t)base_UISkillNameChangeWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISkillNameChangeWnd[] = {(uintptr_t)&type_UISkillNameChangeWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISkillNameChangeWnd};
const uintptr_t locator_UINotifyItemObtainWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINotifyItemObtainWnd, (uintptr_t)hierarchy_UINotifyItemObtainWnd};
const uintptr_t hierarchy_UINotifyItemObtainWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UINotifyItemObtainWnd};
const uintptr_t bases_UINotifyItemObtainWnd[] = {(uintptr_t)base_UINotifyItemObtainWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINotifyItemObtainWnd[] = {(uintptr_t)&type_UINotifyItemObtainWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINotifyItemObtainWnd};
const uintptr_t locator_UINpcEditDialogWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINpcEditDialogWnd, (uintptr_t)hierarchy_UINpcEditDialogWnd};
const uintptr_t hierarchy_UINpcEditDialogWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UINpcEditDialogWnd};
const uintptr_t bases_UINpcEditDialogWnd[] = {(uintptr_t)base_UINpcEditDialogWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINpcEditDialogWnd[] = {(uintptr_t)&type_UINpcEditDialogWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINpcEditDialogWnd};
const uintptr_t locator_UINpcTextEditDialogWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINpcTextEditDialogWnd, (uintptr_t)hierarchy_UINpcTextEditDialogWnd};
const uintptr_t hierarchy_UINpcTextEditDialogWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UINpcTextEditDialogWnd};
const uintptr_t bases_UINpcTextEditDialogWnd[] = {(uintptr_t)base_UINpcTextEditDialogWnd, (uintptr_t)base_UINpcEditDialogWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINpcTextEditDialogWnd[] = {(uintptr_t)&type_UINpcTextEditDialogWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINpcTextEditDialogWnd};
const uintptr_t locator_UIStatusWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIStatusWnd, (uintptr_t)hierarchy_UIStatusWnd};
const uintptr_t hierarchy_UIStatusWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIStatusWnd};
const uintptr_t bases_UIStatusWnd[] = {(uintptr_t)base_UIStatusWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIStatusWnd[] = {(uintptr_t)&type_UIStatusWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIStatusWnd};
const uintptr_t locator_UIItemCollectionWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemCollectionWnd, (uintptr_t)hierarchy_UIItemCollectionWnd};
const uintptr_t hierarchy_UIItemCollectionWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIItemCollectionWnd};
const uintptr_t bases_UIItemCollectionWnd[] = {(uintptr_t)base_UIItemCollectionWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemCollectionWnd[] = {(uintptr_t)&type_UIItemCollectionWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemCollectionWnd};
const uintptr_t locator_UICombinedCardItemCollectionWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UICombinedCardItemCollectionWnd, (uintptr_t)hierarchy_UICombinedCardItemCollectionWnd};
const uintptr_t hierarchy_UICombinedCardItemCollectionWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UICombinedCardItemCollectionWnd};
const uintptr_t bases_UICombinedCardItemCollectionWnd[] = {(uintptr_t)base_UICombinedCardItemCollectionWnd, (uintptr_t)base_UIItemCollectionWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UICombinedCardItemCollectionWnd[] = {(uintptr_t)&type_UICombinedCardItemCollectionWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UICombinedCardItemCollectionWnd};
const uintptr_t locator_UIItemCollection_ComparisonWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemCollection_ComparisonWnd, (uintptr_t)hierarchy_UIItemCollection_ComparisonWnd};
const uintptr_t hierarchy_UIItemCollection_ComparisonWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIItemCollection_ComparisonWnd};
const uintptr_t bases_UIItemCollection_ComparisonWnd[] = {(uintptr_t)base_UIItemCollection_ComparisonWnd, (uintptr_t)base_UIItemCollectionWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemCollection_ComparisonWnd[] = {(uintptr_t)&type_UIItemCollection_ComparisonWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemCollection_ComparisonWnd};
const uintptr_t locator_UICombinedCardItemCollection_ComparisonWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UICombinedCardItemCollection_ComparisonWnd, (uintptr_t)hierarchy_UICombinedCardItemCollection_ComparisonWnd};
const uintptr_t hierarchy_UICombinedCardItemCollection_ComparisonWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UICombinedCardItemCollection_ComparisonWnd};
const uintptr_t bases_UICombinedCardItemCollection_ComparisonWnd[] = {(uintptr_t)base_UICombinedCardItemCollection_ComparisonWnd, (uintptr_t)base_UIItemCollectionWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UICombinedCardItemCollection_ComparisonWnd[] = {(uintptr_t)&type_UICombinedCardItemCollection_ComparisonWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UICombinedCardItemCollection_ComparisonWnd};
const uintptr_t locator_UISkillDescribeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISkillDescribeWnd, (uintptr_t)hierarchy_UISkillDescribeWnd};
const uintptr_t hierarchy_UISkillDescribeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UISkillDescribeWnd};
const uintptr_t bases_UISkillDescribeWnd[] = {(uintptr_t)base_UISkillDescribeWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISkillDescribeWnd[] = {(uintptr_t)&type_UISkillDescribeWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISkillDescribeWnd};
const uintptr_t locator_UICardItemIllustWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UICardItemIllustWnd, (uintptr_t)hierarchy_UICardItemIllustWnd};
const uintptr_t hierarchy_UICardItemIllustWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UICardItemIllustWnd};
const uintptr_t bases_UICardItemIllustWnd[] = {(uintptr_t)base_UICardItemIllustWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UICardItemIllustWnd[] = {(uintptr_t)&type_UICardItemIllustWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UICardItemIllustWnd};
const uintptr_t locator_UIMonsterDisplayWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMonsterDisplayWnd, (uintptr_t)hierarchy_UIMonsterDisplayWnd};
const uintptr_t hierarchy_UIMonsterDisplayWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMonsterDisplayWnd};
const uintptr_t bases_UIMonsterDisplayWnd[] = {(uintptr_t)base_UIMonsterDisplayWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMonsterDisplayWnd[] = {(uintptr_t)&type_UIMonsterDisplayWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMonsterDisplayWnd};
const uintptr_t locator_UISayDialogWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISayDialogWnd, (uintptr_t)hierarchy_UISayDialogWnd};
const uintptr_t hierarchy_UISayDialogWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UISayDialogWnd};
const uintptr_t bases_UISayDialogWnd[] = {(uintptr_t)base_UISayDialogWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISayDialogWnd[] = {(uintptr_t)&type_UISayDialogWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISayDialogWnd};
const uintptr_t locator_UIChooseWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIChooseWnd, (uintptr_t)hierarchy_UIChooseWnd};
const uintptr_t hierarchy_UIChooseWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIChooseWnd};
const uintptr_t bases_UIChooseWnd[] = {(uintptr_t)base_UIChooseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIChooseWnd[] = {(uintptr_t)&type_UIChooseWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIChooseWnd};
const uintptr_t locator_UIChooseWarpWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIChooseWarpWnd, (uintptr_t)hierarchy_UIChooseWarpWnd};
const uintptr_t hierarchy_UIChooseWarpWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIChooseWarpWnd};
const uintptr_t bases_UIChooseWarpWnd[] = {(uintptr_t)base_UIChooseWarpWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIChooseWarpWnd[] = {(uintptr_t)&type_UIChooseWarpWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIChooseWarpWnd};
const uintptr_t locator_UIRestartWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRestartWnd, (uintptr_t)hierarchy_UIRestartWnd};
const uintptr_t hierarchy_UIRestartWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIRestartWnd};
const uintptr_t bases_UIRestartWnd[] = {(uintptr_t)base_UIRestartWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRestartWnd[] = {(uintptr_t)&type_UIRestartWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRestartWnd};
const uintptr_t locator_UIQuitWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIQuitWnd, (uintptr_t)hierarchy_UIQuitWnd};
const uintptr_t hierarchy_UIQuitWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIQuitWnd};
const uintptr_t bases_UIQuitWnd[] = {(uintptr_t)base_UIQuitWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIQuitWnd[] = {(uintptr_t)&type_UIQuitWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIQuitWnd};
const uintptr_t locator_UINoticeConfirmWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINoticeConfirmWnd, (uintptr_t)hierarchy_UINoticeConfirmWnd};
const uintptr_t hierarchy_UINoticeConfirmWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UINoticeConfirmWnd};
const uintptr_t bases_UINoticeConfirmWnd[] = {(uintptr_t)base_UINoticeConfirmWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINoticeConfirmWnd[] = {(uintptr_t)&type_UINoticeConfirmWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINoticeConfirmWnd};
const uintptr_t locator_UIChooseSellBuyWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIChooseSellBuyWnd, (uintptr_t)hierarchy_UIChooseSellBuyWnd};
const uintptr_t hierarchy_UIChooseSellBuyWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIChooseSellBuyWnd};
const uintptr_t bases_UIChooseSellBuyWnd[] = {(uintptr_t)base_UIChooseSellBuyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIChooseSellBuyWnd[] = {(uintptr_t)&type_UIChooseSellBuyWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIChooseSellBuyWnd};
const uintptr_t locator_UIDisconnectedServerMsgWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIDisconnectedServerMsgWnd, (uintptr_t)hierarchy_UIDisconnectedServerMsgWnd};
const uintptr_t hierarchy_UIDisconnectedServerMsgWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIDisconnectedServerMsgWnd};
const uintptr_t bases_UIDisconnectedServerMsgWnd[] = {(uintptr_t)base_UIDisconnectedServerMsgWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIDisconnectedServerMsgWnd[] = {(uintptr_t)&type_UIDisconnectedServerMsgWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIDisconnectedServerMsgWnd};
const uintptr_t locator_UIExchangeAcceptWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIExchangeAcceptWnd, (uintptr_t)hierarchy_UIExchangeAcceptWnd};
const uintptr_t hierarchy_UIExchangeAcceptWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIExchangeAcceptWnd};
const uintptr_t bases_UIExchangeAcceptWnd[] = {(uintptr_t)base_UIExchangeAcceptWnd, (uintptr_t)base_UIChooseSellBuyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIExchangeAcceptWnd[] = {(uintptr_t)&type_UIExchangeAcceptWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIExchangeAcceptWnd};
const uintptr_t locator_UIJoinPartyAcceptWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIJoinPartyAcceptWnd, (uintptr_t)hierarchy_UIJoinPartyAcceptWnd};
const uintptr_t hierarchy_UIJoinPartyAcceptWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_UIJoinPartyAcceptWnd};
const uintptr_t bases_UIJoinPartyAcceptWnd[] = {(uintptr_t)base_UIJoinPartyAcceptWnd, (uintptr_t)base_UIExchangeAcceptWnd, (uintptr_t)base_UIChooseSellBuyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIJoinPartyAcceptWnd[] = {(uintptr_t)&type_UIJoinPartyAcceptWnd, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIJoinPartyAcceptWnd};
const uintptr_t locator_UICoupleAcceptWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UICoupleAcceptWnd, (uintptr_t)hierarchy_UICoupleAcceptWnd};
const uintptr_t hierarchy_UICoupleAcceptWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_UICoupleAcceptWnd};
const uintptr_t bases_UICoupleAcceptWnd[] = {(uintptr_t)base_UICoupleAcceptWnd, (uintptr_t)base_UIExchangeAcceptWnd, (uintptr_t)base_UIChooseSellBuyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UICoupleAcceptWnd[] = {(uintptr_t)&type_UICoupleAcceptWnd, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UICoupleAcceptWnd};
const uintptr_t locator_UIStarPlaceAcceptWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIStarPlaceAcceptWnd, (uintptr_t)hierarchy_UIStarPlaceAcceptWnd};
const uintptr_t hierarchy_UIStarPlaceAcceptWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_UIStarPlaceAcceptWnd};
const uintptr_t bases_UIStarPlaceAcceptWnd[] = {(uintptr_t)base_UIStarPlaceAcceptWnd, (uintptr_t)base_UIExchangeAcceptWnd, (uintptr_t)base_UIChooseSellBuyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIStarPlaceAcceptWnd[] = {(uintptr_t)&type_UIStarPlaceAcceptWnd, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIStarPlaceAcceptWnd};
const uintptr_t locator_UIMixAcceptWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMixAcceptWnd, (uintptr_t)hierarchy_UIMixAcceptWnd};
const uintptr_t hierarchy_UIMixAcceptWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_UIMixAcceptWnd};
const uintptr_t bases_UIMixAcceptWnd[] = {(uintptr_t)base_UIMixAcceptWnd, (uintptr_t)base_UIExchangeAcceptWnd, (uintptr_t)base_UIChooseSellBuyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMixAcceptWnd[] = {(uintptr_t)&type_UIMixAcceptWnd, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMixAcceptWnd};
const uintptr_t locator_UIYourItemWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIYourItemWnd, (uintptr_t)hierarchy_UIYourItemWnd};
const uintptr_t hierarchy_UIYourItemWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_UIYourItemWnd};
const uintptr_t bases_UIYourItemWnd[] = {(uintptr_t)base_UIYourItemWnd, (uintptr_t)base_UIExchangeAcceptWnd, (uintptr_t)base_UIChooseSellBuyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIYourItemWnd[] = {(uintptr_t)&type_UIYourItemWnd, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIYourItemWnd};
const uintptr_t locator_UIBabyAcceptWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBabyAcceptWnd, (uintptr_t)hierarchy_UIBabyAcceptWnd};
const uintptr_t hierarchy_UIBabyAcceptWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_UIBabyAcceptWnd};
const uintptr_t bases_UIBabyAcceptWnd[] = {(uintptr_t)base_UIBabyAcceptWnd, (uintptr_t)base_UIExchangeAcceptWnd, (uintptr_t)base_UIChooseSellBuyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBabyAcceptWnd[] = {(uintptr_t)&type_UIBabyAcceptWnd, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBabyAcceptWnd};
const uintptr_t locator_UIBabyAcceptWnd2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBabyAcceptWnd2, (uintptr_t)hierarchy_UIBabyAcceptWnd2};
const uintptr_t hierarchy_UIBabyAcceptWnd2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_UIBabyAcceptWnd2};
const uintptr_t bases_UIBabyAcceptWnd2[] = {(uintptr_t)base_UIBabyAcceptWnd2, (uintptr_t)base_UIExchangeAcceptWnd, (uintptr_t)base_UIChooseSellBuyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBabyAcceptWnd2[] = {(uintptr_t)&type_UIBabyAcceptWnd2, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBabyAcceptWnd2};
const uintptr_t locator_UIJoinGuildAcceptWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIJoinGuildAcceptWnd, (uintptr_t)hierarchy_UIJoinGuildAcceptWnd};
const uintptr_t hierarchy_UIJoinGuildAcceptWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_UIJoinGuildAcceptWnd};
const uintptr_t bases_UIJoinGuildAcceptWnd[] = {(uintptr_t)base_UIJoinGuildAcceptWnd, (uintptr_t)base_UIExchangeAcceptWnd, (uintptr_t)base_UIChooseSellBuyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIJoinGuildAcceptWnd[] = {(uintptr_t)&type_UIJoinGuildAcceptWnd, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIJoinGuildAcceptWnd};
const uintptr_t locator_UITipOfTheDayWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UITipOfTheDayWnd, (uintptr_t)hierarchy_UITipOfTheDayWnd};
const uintptr_t hierarchy_UITipOfTheDayWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UITipOfTheDayWnd};
const uintptr_t bases_UITipOfTheDayWnd[] = {(uintptr_t)base_UITipOfTheDayWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UITipOfTheDayWnd[] = {(uintptr_t)&type_UITipOfTheDayWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UITipOfTheDayWnd};
const uintptr_t locator_UIChatRoomMakeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIChatRoomMakeWnd, (uintptr_t)hierarchy_UIChatRoomMakeWnd};
const uintptr_t hierarchy_UIChatRoomMakeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIChatRoomMakeWnd};
const uintptr_t bases_UIChatRoomMakeWnd[] = {(uintptr_t)base_UIChatRoomMakeWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIChatRoomMakeWnd[] = {(uintptr_t)&type_UIChatRoomMakeWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIChatRoomMakeWnd};
const uintptr_t locator_UIChatRoomChangeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIChatRoomChangeWnd, (uintptr_t)hierarchy_UIChatRoomChangeWnd};
const uintptr_t hierarchy_UIChatRoomChangeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIChatRoomChangeWnd};
const uintptr_t bases_UIChatRoomChangeWnd[] = {(uintptr_t)base_UIChatRoomChangeWnd, (uintptr_t)base_UIChatRoomMakeWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIChatRoomChangeWnd[] = {(uintptr_t)&type_UIChatRoomChangeWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIChatRoomChangeWnd};
const uintptr_t locator_UIPartySettingWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIPartySettingWnd, (uintptr_t)hierarchy_UIPartySettingWnd};
const uintptr_t hierarchy_UIPartySettingWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIPartySettingWnd};
const uintptr_t bases_UIPartySettingWnd[] = {(uintptr_t)base_UIPartySettingWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIPartySettingWnd[] = {(uintptr_t)&type_UIPartySettingWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIPartySettingWnd};
const uintptr_t locator_UIPartyInvitationToWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIPartyInvitationToWnd, (uintptr_t)hierarchy_UIPartyInvitationToWnd};
const uintptr_t hierarchy_UIPartyInvitationToWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIPartyInvitationToWnd};
const uintptr_t bases_UIPartyInvitationToWnd[] = {(uintptr_t)base_UIPartyInvitationToWnd, (uintptr_t)base_UIPartySettingWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIPartyInvitationToWnd[] = {(uintptr_t)&type_UIPartyInvitationToWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIPartyInvitationToWnd};
const uintptr_t locator_UIWhisperWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIWhisperWnd, (uintptr_t)hierarchy_UIWhisperWnd};
const uintptr_t hierarchy_UIWhisperWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIWhisperWnd};
const uintptr_t bases_UIWhisperWnd[] = {(uintptr_t)base_UIWhisperWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIWhisperWnd[] = {(uintptr_t)&type_UIWhisperWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIWhisperWnd};
const uintptr_t locator_UIItemIdentifyWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemIdentifyWnd, (uintptr_t)hierarchy_UIItemIdentifyWnd};
const uintptr_t hierarchy_UIItemIdentifyWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIItemIdentifyWnd};
const uintptr_t bases_UIItemIdentifyWnd[] = {(uintptr_t)base_UIItemIdentifyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemIdentifyWnd[] = {(uintptr_t)&type_UIItemIdentifyWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemIdentifyWnd};
const uintptr_t locator_UIPetEggListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIPetEggListWnd, (uintptr_t)hierarchy_UIPetEggListWnd};
const uintptr_t hierarchy_UIPetEggListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIPetEggListWnd};
const uintptr_t bases_UIPetEggListWnd[] = {(uintptr_t)base_UIPetEggListWnd, (uintptr_t)base_UIItemIdentifyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIPetEggListWnd[] = {(uintptr_t)&type_UIPetEggListWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIPetEggListWnd};
const uintptr_t locator_UIMakingArrowListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMakingArrowListWnd, (uintptr_t)hierarchy_UIMakingArrowListWnd};
const uintptr_t hierarchy_UIMakingArrowListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIMakingArrowListWnd};
const uintptr_t bases_UIMakingArrowListWnd[] = {(uintptr_t)base_UIMakingArrowListWnd, (uintptr_t)base_UIItemIdentifyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMakingArrowListWnd[] = {(uintptr_t)&type_UIMakingArrowListWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMakingArrowListWnd};
const uintptr_t locator_UIItemCompositionWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemCompositionWnd, (uintptr_t)hierarchy_UIItemCompositionWnd};
const uintptr_t hierarchy_UIItemCompositionWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIItemCompositionWnd};
const uintptr_t bases_UIItemCompositionWnd[] = {(uintptr_t)base_UIItemCompositionWnd, (uintptr_t)base_UIItemIdentifyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemCompositionWnd[] = {(uintptr_t)&type_UIItemCompositionWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemCompositionWnd};
const uintptr_t locator_UIMenuWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMenuWnd, (uintptr_t)hierarchy_UIMenuWnd};
const uintptr_t hierarchy_UIMenuWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMenuWnd};
const uintptr_t bases_UIMenuWnd[] = {(uintptr_t)base_UIMenuWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMenuWnd[] = {(uintptr_t)&type_UIMenuWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMenuWnd};
const uintptr_t locator_UIComboBoxWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIComboBoxWnd, (uintptr_t)hierarchy_UIComboBoxWnd};
const uintptr_t hierarchy_UIComboBoxWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIComboBoxWnd};
const uintptr_t bases_UIComboBoxWnd[] = {(uintptr_t)base_UIComboBoxWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIComboBoxWnd[] = {(uintptr_t)&type_UIComboBoxWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIComboBoxWnd};
const uintptr_t locator_UIComboBoxIDWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIComboBoxIDWnd, (uintptr_t)hierarchy_UIComboBoxIDWnd};
const uintptr_t hierarchy_UIComboBoxIDWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIComboBoxIDWnd};
const uintptr_t bases_UIComboBoxIDWnd[] = {(uintptr_t)base_UIComboBoxIDWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIComboBoxIDWnd[] = {(uintptr_t)&type_UIComboBoxIDWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIComboBoxIDWnd};
const uintptr_t locator_UIPetTamingDeceiveWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIPetTamingDeceiveWnd, (uintptr_t)hierarchy_UIPetTamingDeceiveWnd};
const uintptr_t hierarchy_UIPetTamingDeceiveWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIPetTamingDeceiveWnd};
const uintptr_t bases_UIPetTamingDeceiveWnd[] = {(uintptr_t)base_UIPetTamingDeceiveWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIPetTamingDeceiveWnd[] = {(uintptr_t)&type_UIPetTamingDeceiveWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIPetTamingDeceiveWnd};
const uintptr_t locator_UISelectCartWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISelectCartWnd, (uintptr_t)hierarchy_UISelectCartWnd};
const uintptr_t hierarchy_UISelectCartWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UISelectCartWnd};
const uintptr_t bases_UISelectCartWnd[] = {(uintptr_t)base_UISelectCartWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISelectCartWnd[] = {(uintptr_t)&type_UISelectCartWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISelectCartWnd};
const uintptr_t locator_UISpellListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISpellListWnd, (uintptr_t)hierarchy_UISpellListWnd};
const uintptr_t hierarchy_UISpellListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UISpellListWnd};
const uintptr_t bases_UISpellListWnd[] = {(uintptr_t)base_UISpellListWnd, (uintptr_t)base_UIItemIdentifyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISpellListWnd[] = {(uintptr_t)&type_UISpellListWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISpellListWnd};
const uintptr_t locator_UIGraffiStrboxWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIGraffiStrboxWnd, (uintptr_t)hierarchy_UIGraffiStrboxWnd};
const uintptr_t hierarchy_UIGraffiStrboxWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIGraffiStrboxWnd};
const uintptr_t bases_UIGraffiStrboxWnd[] = {(uintptr_t)base_UIGraffiStrboxWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIGraffiStrboxWnd[] = {(uintptr_t)&type_UIGraffiStrboxWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIGraffiStrboxWnd};
const uintptr_t locator_UIProhibitListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIProhibitListWnd, (uintptr_t)hierarchy_UIProhibitListWnd};
const uintptr_t hierarchy_UIProhibitListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIProhibitListWnd};
const uintptr_t bases_UIProhibitListWnd[] = {(uintptr_t)base_UIProhibitListWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIProhibitListWnd[] = {(uintptr_t)&type_UIProhibitListWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIProhibitListWnd};
const uintptr_t locator_UIBookWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBookWnd, (uintptr_t)hierarchy_UIBookWnd};
const uintptr_t hierarchy_UIBookWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIBookWnd};
const uintptr_t bases_UIBookWnd[] = {(uintptr_t)base_UIBookWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBookWnd[] = {(uintptr_t)&type_UIBookWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBookWnd};
const uintptr_t locator_UIJoinFriendAcceptWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIJoinFriendAcceptWnd, (uintptr_t)hierarchy_UIJoinFriendAcceptWnd};
const uintptr_t hierarchy_UIJoinFriendAcceptWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_UIJoinFriendAcceptWnd};
const uintptr_t bases_UIJoinFriendAcceptWnd[] = {(uintptr_t)base_UIJoinFriendAcceptWnd, (uintptr_t)base_UIExchangeAcceptWnd, (uintptr_t)base_UIChooseSellBuyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIJoinFriendAcceptWnd[] = {(uintptr_t)&type_UIJoinFriendAcceptWnd, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIJoinFriendAcceptWnd};
const uintptr_t locator_UICommandListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UICommandListWnd, (uintptr_t)hierarchy_UICommandListWnd};
const uintptr_t hierarchy_UICommandListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UICommandListWnd};
const uintptr_t bases_UICommandListWnd[] = {(uintptr_t)base_UICommandListWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UICommandListWnd[] = {(uintptr_t)&type_UICommandListWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UICommandListWnd};
const uintptr_t locator_UIStorePasswordInputWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIStorePasswordInputWnd, (uintptr_t)hierarchy_UIStorePasswordInputWnd};
const uintptr_t hierarchy_UIStorePasswordInputWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIStorePasswordInputWnd};
const uintptr_t bases_UIStorePasswordInputWnd[] = {(uintptr_t)base_UIStorePasswordInputWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIStorePasswordInputWnd[] = {(uintptr_t)&type_UIStorePasswordInputWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIStorePasswordInputWnd};
const uintptr_t locator_UISecondPasswordInputWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISecondPasswordInputWnd, (uintptr_t)hierarchy_UISecondPasswordInputWnd};
const uintptr_t hierarchy_UISecondPasswordInputWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UISecondPasswordInputWnd};
const uintptr_t bases_UISecondPasswordInputWnd[] = {(uintptr_t)base_UISecondPasswordInputWnd, (uintptr_t)base_UIStorePasswordInputWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISecondPasswordInputWnd[] = {(uintptr_t)&type_UISecondPasswordInputWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISecondPasswordInputWnd};
const uintptr_t locator_UIAutoMessageWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIAutoMessageWnd, (uintptr_t)hierarchy_UIAutoMessageWnd};
const uintptr_t hierarchy_UIAutoMessageWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIAutoMessageWnd};
const uintptr_t bases_UIAutoMessageWnd[] = {(uintptr_t)base_UIAutoMessageWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIAutoMessageWnd[] = {(uintptr_t)&type_UIAutoMessageWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIAutoMessageWnd};
const uintptr_t locator_UITipboxWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UITipboxWnd, (uintptr_t)hierarchy_UITipboxWnd};
const uintptr_t hierarchy_UITipboxWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UITipboxWnd};
const uintptr_t bases_UITipboxWnd[] = {(uintptr_t)base_UITipboxWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UITipboxWnd[] = {(uintptr_t)&type_UITipboxWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UITipboxWnd};
const uintptr_t locator_UIShortCutWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIShortCutWnd, (uintptr_t)hierarchy_UIShortCutWnd};
const uintptr_t hierarchy_UIShortCutWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIShortCutWnd};
const uintptr_t bases_UIShortCutWnd[] = {(uintptr_t)base_UIShortCutWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIShortCutWnd[] = {(uintptr_t)&type_UIShortCutWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIShortCutWnd};
const uintptr_t locator_UICharInfoBalloonText[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UICharInfoBalloonText, (uintptr_t)hierarchy_UICharInfoBalloonText};
const uintptr_t hierarchy_UICharInfoBalloonText[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UICharInfoBalloonText};
const uintptr_t bases_UICharInfoBalloonText[] = {(uintptr_t)base_UICharInfoBalloonText, (uintptr_t)base_UITransBalloonText, (uintptr_t)base_UIBalloonText, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UICharInfoBalloonText[] = {(uintptr_t)&type_UICharInfoBalloonText, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UICharInfoBalloonText};
const uintptr_t locator_UISelCharForUServerWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISelCharForUServerWnd, (uintptr_t)hierarchy_UISelCharForUServerWnd};
const uintptr_t hierarchy_UISelCharForUServerWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UISelCharForUServerWnd};
const uintptr_t bases_UISelCharForUServerWnd[] = {(uintptr_t)base_UISelCharForUServerWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISelCharForUServerWnd[] = {(uintptr_t)&type_UISelCharForUServerWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISelCharForUServerWnd};
const uintptr_t locator_UIPartyBookingHelpWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIPartyBookingHelpWnd, (uintptr_t)hierarchy_UIPartyBookingHelpWnd};
const uintptr_t hierarchy_UIPartyBookingHelpWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIPartyBookingHelpWnd};
const uintptr_t bases_UIPartyBookingHelpWnd[] = {(uintptr_t)base_UIPartyBookingHelpWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIPartyBookingHelpWnd[] = {(uintptr_t)&type_UIPartyBookingHelpWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIPartyBookingHelpWnd};
const uintptr_t locator_UISeekPartyWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISeekPartyWnd, (uintptr_t)hierarchy_UISeekPartyWnd};
const uintptr_t hierarchy_UISeekPartyWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UISeekPartyWnd};
const uintptr_t bases_UISeekPartyWnd[] = {(uintptr_t)base_UISeekPartyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISeekPartyWnd[] = {(uintptr_t)&type_UISeekPartyWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISeekPartyWnd};
const uintptr_t locator_UIJobListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIJobListWnd, (uintptr_t)hierarchy_UIJobListWnd};
const uintptr_t hierarchy_UIJobListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIJobListWnd};
const uintptr_t bases_UIJobListWnd[] = {(uintptr_t)base_UIJobListWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIJobListWnd[] = {(uintptr_t)&type_UIJobListWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIJobListWnd};
const uintptr_t locator_UISeekPartyMBWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISeekPartyMBWnd, (uintptr_t)hierarchy_UISeekPartyMBWnd};
const uintptr_t hierarchy_UISeekPartyMBWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UISeekPartyMBWnd};
const uintptr_t bases_UISeekPartyMBWnd[] = {(uintptr_t)base_UISeekPartyMBWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISeekPartyMBWnd[] = {(uintptr_t)&type_UISeekPartyMBWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISeekPartyMBWnd};
const uintptr_t locator_UISeekPartyListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISeekPartyListWnd, (uintptr_t)hierarchy_UISeekPartyListWnd};
const uintptr_t hierarchy_UISeekPartyListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UISeekPartyListWnd};
const uintptr_t bases_UISeekPartyListWnd[] = {(uintptr_t)base_UISeekPartyListWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISeekPartyListWnd[] = {(uintptr_t)&type_UISeekPartyListWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISeekPartyListWnd};
const uintptr_t locator_UIChangeNameWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIChangeNameWnd, (uintptr_t)hierarchy_UIChangeNameWnd};
const uintptr_t hierarchy_UIChangeNameWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIChangeNameWnd};
const uintptr_t bases_UIChangeNameWnd[] = {(uintptr_t)base_UIChangeNameWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIChangeNameWnd[] = {(uintptr_t)&type_UIChangeNameWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIChangeNameWnd};
const uintptr_t locator_UIRebirthWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRebirthWnd, (uintptr_t)hierarchy_UIRebirthWnd};
const uintptr_t hierarchy_UIRebirthWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIRebirthWnd};
const uintptr_t bases_UIRebirthWnd[] = {(uintptr_t)base_UIRebirthWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRebirthWnd[] = {(uintptr_t)&type_UIRebirthWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRebirthWnd};
const uintptr_t locator_UISubChatMiniWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISubChatMiniWnd, (uintptr_t)hierarchy_UISubChatMiniWnd};
const uintptr_t hierarchy_UISubChatMiniWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UISubChatMiniWnd};
const uintptr_t bases_UISubChatMiniWnd[] = {(uintptr_t)base_UISubChatMiniWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISubChatMiniWnd[] = {(uintptr_t)&type_UISubChatMiniWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISubChatMiniWnd};
const uintptr_t locator_UIBattleMsgOptionWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBattleMsgOptionWnd, (uintptr_t)hierarchy_UIBattleMsgOptionWnd};
const uintptr_t hierarchy_UIBattleMsgOptionWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIBattleMsgOptionWnd};
const uintptr_t bases_UIBattleMsgOptionWnd[] = {(uintptr_t)base_UIBattleMsgOptionWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBattleMsgOptionWnd[] = {(uintptr_t)&type_UIBattleMsgOptionWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBattleMsgOptionWnd};
const uintptr_t locator_UISubChatHisWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISubChatHisWnd, (uintptr_t)hierarchy_UISubChatHisWnd};
const uintptr_t hierarchy_UISubChatHisWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UISubChatHisWnd};
const uintptr_t bases_UISubChatHisWnd[] = {(uintptr_t)base_UISubChatHisWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISubChatHisWnd[] = {(uintptr_t)&type_UISubChatHisWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISubChatHisWnd};
const uintptr_t locator_UINewChatWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINewChatWnd, (uintptr_t)hierarchy_UINewChatWnd};
const uintptr_t hierarchy_UINewChatWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UINewChatWnd};
const uintptr_t bases_UINewChatWnd[] = {(uintptr_t)base_UINewChatWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINewChatWnd[] = {(uintptr_t)&type_UINewChatWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINewChatWnd};
const uintptr_t locator_UIMemorialDunWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMemorialDunWnd, (uintptr_t)hierarchy_UIMemorialDunWnd};
const uintptr_t hierarchy_UIMemorialDunWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMemorialDunWnd};
const uintptr_t bases_UIMemorialDunWnd[] = {(uintptr_t)base_UIMemorialDunWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMemorialDunWnd[] = {(uintptr_t)&type_UIMemorialDunWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMemorialDunWnd};
const uintptr_t locator_UIRoMapWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRoMapWnd, (uintptr_t)hierarchy_UIRoMapWnd};
const uintptr_t hierarchy_UIRoMapWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIRoMapWnd};
const uintptr_t bases_UIRoMapWnd[] = {(uintptr_t)base_UIRoMapWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRoMapWnd[] = {(uintptr_t)&type_UIRoMapWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRoMapWnd};
const uintptr_t locator_UIMiniBtnWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMiniBtnWnd, (uintptr_t)hierarchy_UIMiniBtnWnd};
const uintptr_t hierarchy_UIMiniBtnWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMiniBtnWnd};
const uintptr_t bases_UIMiniBtnWnd[] = {(uintptr_t)base_UIMiniBtnWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMiniBtnWnd[] = {(uintptr_t)&type_UIMiniBtnWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMiniBtnWnd};
const uintptr_t locator_UIHotKeyWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIHotKeyWnd, (uintptr_t)hierarchy_UIHotKeyWnd};
const uintptr_t hierarchy_UIHotKeyWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIHotKeyWnd};
const uintptr_t bases_UIHotKeyWnd[] = {(uintptr_t)base_UIHotKeyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIHotKeyWnd[] = {(uintptr_t)&type_UIHotKeyWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIHotKeyWnd};
const uintptr_t locator_UIEscOptionWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEscOptionWnd, (uintptr_t)hierarchy_UIEscOptionWnd};
const uintptr_t hierarchy_UIEscOptionWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIEscOptionWnd};
const uintptr_t bases_UIEscOptionWnd[] = {(uintptr_t)base_UIEscOptionWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEscOptionWnd[] = {(uintptr_t)&type_UIEscOptionWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEscOptionWnd};
const uintptr_t locator_UIQuikSlotWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIQuikSlotWnd, (uintptr_t)hierarchy_UIQuikSlotWnd};
const uintptr_t hierarchy_UIQuikSlotWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIQuikSlotWnd};
const uintptr_t bases_UIQuikSlotWnd[] = {(uintptr_t)base_UIQuikSlotWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIQuikSlotWnd[] = {(uintptr_t)&type_UIQuikSlotWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIQuikSlotWnd};
const uintptr_t locator_UISerachStoreInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISerachStoreInfoWnd, (uintptr_t)hierarchy_UISerachStoreInfoWnd};
const uintptr_t hierarchy_UISerachStoreInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UISerachStoreInfoWnd};
const uintptr_t bases_UISerachStoreInfoWnd[] = {(uintptr_t)base_UISerachStoreInfoWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISerachStoreInfoWnd[] = {(uintptr_t)&type_UISerachStoreInfoWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISerachStoreInfoWnd};
const uintptr_t locator_UIButtonsOfBasicWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIButtonsOfBasicWnd, (uintptr_t)hierarchy_UIButtonsOfBasicWnd};
const uintptr_t hierarchy_UIButtonsOfBasicWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIButtonsOfBasicWnd};
const uintptr_t bases_UIButtonsOfBasicWnd[] = {(uintptr_t)base_UIButtonsOfBasicWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIButtonsOfBasicWnd[] = {(uintptr_t)&type_UIButtonsOfBasicWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIButtonsOfBasicWnd};
const uintptr_t locator_UINavigationWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINavigationWnd, (uintptr_t)hierarchy_UINavigationWnd};
const uintptr_t hierarchy_UINavigationWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UINavigationWnd};
const uintptr_t bases_UINavigationWnd[] = {(uintptr_t)base_UINavigationWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINavigationWnd[] = {(uintptr_t)&type_UINavigationWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINavigationWnd};
const uintptr_t locator_UINavigationHelpWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINavigationHelpWnd, (uintptr_t)hierarchy_UINavigationHelpWnd};
const uintptr_t hierarchy_UINavigationHelpWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UINavigationHelpWnd};
const uintptr_t bases_UINavigationHelpWnd[] = {(uintptr_t)base_UINavigationHelpWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINavigationHelpWnd[] = {(uintptr_t)&type_UINavigationHelpWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINavigationHelpWnd};
const uintptr_t locator_CUICollectionSystemWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CUICollectionSystemWnd, (uintptr_t)hierarchy_CUICollectionSystemWnd};
const uintptr_t hierarchy_CUICollectionSystemWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_CUICollectionSystemWnd};
const uintptr_t bases_CUICollectionSystemWnd[] = {(uintptr_t)base_CUICollectionSystemWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_CUICollectionSystemWnd[] = {(uintptr_t)&type_CUICollectionSystemWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CUICollectionSystemWnd};
const uintptr_t locator_UIGuildInfoManageWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIGuildInfoManageWnd, (uintptr_t)hierarchy_UIGuildInfoManageWnd};
const uintptr_t hierarchy_UIGuildInfoManageWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIGuildInfoManageWnd};
const uintptr_t bases_UIGuildInfoManageWnd[] = {(uintptr_t)base_UIGuildInfoManageWnd, (uintptr_t)base_UIGuildWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIGuildInfoManageWnd[] = {(uintptr_t)&type_UIGuildInfoManageWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIGuildInfoManageWnd};
const uintptr_t base_UIGuildWnd[] = {(uintptr_t)&type_UIGuildWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIGuildWnd};
const uintptr_t hierarchy_UIGuildWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIGuildWnd};
const uintptr_t bases_UIGuildWnd[] = {(uintptr_t)base_UIGuildWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t locator_UIGuildMemberManageWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIGuildMemberManageWnd, (uintptr_t)hierarchy_UIGuildMemberManageWnd};
const uintptr_t hierarchy_UIGuildMemberManageWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIGuildMemberManageWnd};
const uintptr_t bases_UIGuildMemberManageWnd[] = {(uintptr_t)base_UIGuildMemberManageWnd, (uintptr_t)base_UIGuildWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIGuildMemberManageWnd[] = {(uintptr_t)&type_UIGuildMemberManageWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIGuildMemberManageWnd};
const uintptr_t locator_UIGuildPositionManageWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIGuildPositionManageWnd, (uintptr_t)hierarchy_UIGuildPositionManageWnd};
const uintptr_t hierarchy_UIGuildPositionManageWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIGuildPositionManageWnd};
const uintptr_t bases_UIGuildPositionManageWnd[] = {(uintptr_t)base_UIGuildPositionManageWnd, (uintptr_t)base_UIGuildWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIGuildPositionManageWnd[] = {(uintptr_t)&type_UIGuildPositionManageWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIGuildPositionManageWnd};
const uintptr_t locator_UIGuildSkillWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIGuildSkillWnd, (uintptr_t)hierarchy_UIGuildSkillWnd};
const uintptr_t hierarchy_UIGuildSkillWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIGuildSkillWnd};
const uintptr_t bases_UIGuildSkillWnd[] = {(uintptr_t)base_UIGuildSkillWnd, (uintptr_t)base_UIGuildWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIGuildSkillWnd[] = {(uintptr_t)&type_UIGuildSkillWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIGuildSkillWnd};
const uintptr_t locator_UIGuildBanishedMemberWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIGuildBanishedMemberWnd, (uintptr_t)hierarchy_UIGuildBanishedMemberWnd};
const uintptr_t hierarchy_UIGuildBanishedMemberWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIGuildBanishedMemberWnd};
const uintptr_t bases_UIGuildBanishedMemberWnd[] = {(uintptr_t)base_UIGuildBanishedMemberWnd, (uintptr_t)base_UIGuildWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIGuildBanishedMemberWnd[] = {(uintptr_t)&type_UIGuildBanishedMemberWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIGuildBanishedMemberWnd};
const uintptr_t locator_UIGuildNoticeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIGuildNoticeWnd, (uintptr_t)hierarchy_UIGuildNoticeWnd};
const uintptr_t hierarchy_UIGuildNoticeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIGuildNoticeWnd};
const uintptr_t bases_UIGuildNoticeWnd[] = {(uintptr_t)base_UIGuildNoticeWnd, (uintptr_t)base_UIGuildWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIGuildNoticeWnd[] = {(uintptr_t)&type_UIGuildNoticeWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIGuildNoticeWnd};
const uintptr_t locator_UIGuildTotalInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIGuildTotalInfoWnd, (uintptr_t)hierarchy_UIGuildTotalInfoWnd};
const uintptr_t hierarchy_UIGuildTotalInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIGuildTotalInfoWnd};
const uintptr_t bases_UIGuildTotalInfoWnd[] = {(uintptr_t)base_UIGuildTotalInfoWnd, (uintptr_t)base_UIGuildWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIGuildTotalInfoWnd[] = {(uintptr_t)&type_UIGuildTotalInfoWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIGuildTotalInfoWnd};
const uintptr_t locator_UIGuildLeaveReasonDescWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIGuildLeaveReasonDescWnd, (uintptr_t)hierarchy_UIGuildLeaveReasonDescWnd};
const uintptr_t hierarchy_UIGuildLeaveReasonDescWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIGuildLeaveReasonDescWnd};
const uintptr_t bases_UIGuildLeaveReasonDescWnd[] = {(uintptr_t)base_UIGuildLeaveReasonDescWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIGuildLeaveReasonDescWnd[] = {(uintptr_t)&type_UIGuildLeaveReasonDescWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIGuildLeaveReasonDescWnd};
const uintptr_t locator_UIGuildBanReasonDescWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIGuildBanReasonDescWnd, (uintptr_t)hierarchy_UIGuildBanReasonDescWnd};
const uintptr_t hierarchy_UIGuildBanReasonDescWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIGuildBanReasonDescWnd};
const uintptr_t bases_UIGuildBanReasonDescWnd[] = {(uintptr_t)base_UIGuildBanReasonDescWnd, (uintptr_t)base_UIGuildLeaveReasonDescWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIGuildBanReasonDescWnd[] = {(uintptr_t)&type_UIGuildBanReasonDescWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIGuildBanReasonDescWnd};
const uintptr_t locator_UIGuildHelperWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIGuildHelperWnd, (uintptr_t)hierarchy_UIGuildHelperWnd};
const uintptr_t hierarchy_UIGuildHelperWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIGuildHelperWnd};
const uintptr_t bases_UIGuildHelperWnd[] = {(uintptr_t)base_UIGuildHelperWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIGuildHelperWnd[] = {(uintptr_t)&type_UIGuildHelperWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIGuildHelperWnd};
const uintptr_t locator_UIGuildTipWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIGuildTipWnd, (uintptr_t)hierarchy_UIGuildTipWnd};
const uintptr_t hierarchy_UIGuildTipWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIGuildTipWnd};
const uintptr_t bases_UIGuildTipWnd[] = {(uintptr_t)base_UIGuildTipWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIGuildTipWnd[] = {(uintptr_t)&type_UIGuildTipWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIGuildTipWnd};
const uintptr_t locator_UICreateGuildWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UICreateGuildWnd, (uintptr_t)hierarchy_UICreateGuildWnd};
const uintptr_t hierarchy_UICreateGuildWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UICreateGuildWnd};
const uintptr_t bases_UICreateGuildWnd[] = {(uintptr_t)base_UICreateGuildWnd, (uintptr_t)base_UIGuildHelperWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UICreateGuildWnd[] = {(uintptr_t)&type_UICreateGuildWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UICreateGuildWnd};
const uintptr_t locator_UIAgitMessageBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIAgitMessageBox, (uintptr_t)hierarchy_UIAgitMessageBox};
const uintptr_t hierarchy_UIAgitMessageBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIAgitMessageBox};
const uintptr_t bases_UIAgitMessageBox[] = {(uintptr_t)base_UIAgitMessageBox, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIAgitMessageBox[] = {(uintptr_t)&type_UIAgitMessageBox, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIAgitMessageBox};
const uintptr_t locator_UIListBoxAgitMenu[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIListBoxAgitMenu, (uintptr_t)hierarchy_UIListBoxAgitMenu};
const uintptr_t hierarchy_UIListBoxAgitMenu[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIListBoxAgitMenu};
const uintptr_t bases_UIListBoxAgitMenu[] = {(uintptr_t)base_UIListBoxAgitMenu, (uintptr_t)base_UIListBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIListBoxAgitMenu[] = {(uintptr_t)&type_UIListBoxAgitMenu, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIListBoxAgitMenu};
const uintptr_t locator_UIListBoxAgit[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIListBoxAgit, (uintptr_t)hierarchy_UIListBoxAgit};
const uintptr_t hierarchy_UIListBoxAgit[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIListBoxAgit};
const uintptr_t bases_UIListBoxAgit[] = {(uintptr_t)base_UIListBoxAgit, (uintptr_t)base_UIListBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIListBoxAgit[] = {(uintptr_t)&type_UIListBoxAgit, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIListBoxAgit};
const uintptr_t locator_UIGuildWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIGuildWnd, (uintptr_t)hierarchy_UIGuildWnd};
const uintptr_t locator_UIEditCtrlWheel[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEditCtrlWheel, (uintptr_t)hierarchy_UIEditCtrlWheel};
const uintptr_t hierarchy_UIEditCtrlWheel[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIEditCtrlWheel};
const uintptr_t bases_UIEditCtrlWheel[] = {(uintptr_t)base_UIEditCtrlWheel, (uintptr_t)base_UIEditCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEditCtrlWheel[] = {(uintptr_t)&type_UIEditCtrlWheel, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEditCtrlWheel};
const uintptr_t locator_UIItemBaseWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemBaseWnd, (uintptr_t)hierarchy_UIItemBaseWnd};
const uintptr_t locator_UIItemStoreSubWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemStoreSubWnd, (uintptr_t)hierarchy_UIItemStoreSubWnd};
const uintptr_t hierarchy_UIItemStoreSubWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIItemStoreSubWnd};
const uintptr_t bases_UIItemStoreSubWnd[] = {(uintptr_t)base_UIItemStoreSubWnd, (uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemStoreSubWnd[] = {(uintptr_t)&type_UIItemStoreSubWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemStoreSubWnd};
const uintptr_t locator_UIItemStoreFindWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemStoreFindWnd, (uintptr_t)hierarchy_UIItemStoreFindWnd};
const uintptr_t hierarchy_UIItemStoreFindWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIItemStoreFindWnd};
const uintptr_t bases_UIItemStoreFindWnd[] = {(uintptr_t)base_UIItemStoreFindWnd, (uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemStoreFindWnd[] = {(uintptr_t)&type_UIItemStoreFindWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemStoreFindWnd};
const uintptr_t locator_UIItemStoreWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemStoreWnd, (uintptr_t)hierarchy_UIItemStoreWnd};
const uintptr_t hierarchy_UIItemStoreWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIItemStoreWnd};
const uintptr_t bases_UIItemStoreWnd[] = {(uintptr_t)base_UIItemStoreWnd, (uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemStoreWnd[] = {(uintptr_t)&type_UIItemStoreWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemStoreWnd};
const uintptr_t locator_UISkillListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISkillListWnd, (uintptr_t)hierarchy_UISkillListWnd};
const uintptr_t hierarchy_UISkillListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UISkillListWnd};
const uintptr_t bases_UISkillListWnd[] = {(uintptr_t)base_UISkillListWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISkillListWnd[] = {(uintptr_t)&type_UISkillListWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISkillListWnd};
const uintptr_t locator_UIItemShopWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemShopWnd, (uintptr_t)hierarchy_UIItemShopWnd};
const uintptr_t hierarchy_UIItemShopWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIItemShopWnd};
const uintptr_t bases_UIItemShopWnd[] = {(uintptr_t)base_UIItemShopWnd, (uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemShopWnd[] = {(uintptr_t)&type_UIItemShopWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemShopWnd};
const uintptr_t locator_UIItemShopWnd2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemShopWnd2, (uintptr_t)hierarchy_UIItemShopWnd2};
const uintptr_t hierarchy_UIItemShopWnd2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIItemShopWnd2};
const uintptr_t bases_UIItemShopWnd2[] = {(uintptr_t)base_UIItemShopWnd2, (uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemShopWnd2[] = {(uintptr_t)&type_UIItemShopWnd2, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemShopWnd2};
const uintptr_t locator_UIItemPurchaseWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemPurchaseWnd, (uintptr_t)hierarchy_UIItemPurchaseWnd};
const uintptr_t hierarchy_UIItemPurchaseWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIItemPurchaseWnd};
const uintptr_t bases_UIItemPurchaseWnd[] = {(uintptr_t)base_UIItemPurchaseWnd, (uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemPurchaseWnd[] = {(uintptr_t)&type_UIItemPurchaseWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemPurchaseWnd};
const uintptr_t locator_UIItemSellWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemSellWnd, (uintptr_t)hierarchy_UIItemSellWnd};
const uintptr_t hierarchy_UIItemSellWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIItemSellWnd};
const uintptr_t bases_UIItemSellWnd[] = {(uintptr_t)base_UIItemSellWnd, (uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemSellWnd[] = {(uintptr_t)&type_UIItemSellWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemSellWnd};
const uintptr_t locator_UIItemSellWnd2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemSellWnd2, (uintptr_t)hierarchy_UIItemSellWnd2};
const uintptr_t hierarchy_UIItemSellWnd2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIItemSellWnd2};
const uintptr_t bases_UIItemSellWnd2[] = {(uintptr_t)base_UIItemSellWnd2, (uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemSellWnd2[] = {(uintptr_t)&type_UIItemSellWnd2, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemSellWnd2};
const uintptr_t locator_UIMerchantItemShopWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMerchantItemShopWnd, (uintptr_t)hierarchy_UIMerchantItemShopWnd};
const uintptr_t hierarchy_UIMerchantItemShopWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIMerchantItemShopWnd};
const uintptr_t bases_UIMerchantItemShopWnd[] = {(uintptr_t)base_UIMerchantItemShopWnd, (uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMerchantItemShopWnd[] = {(uintptr_t)&type_UIMerchantItemShopWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMerchantItemShopWnd};
const uintptr_t locator_UIMerchantItemMyShopWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMerchantItemMyShopWnd, (uintptr_t)hierarchy_UIMerchantItemMyShopWnd};
const uintptr_t hierarchy_UIMerchantItemMyShopWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIMerchantItemMyShopWnd};
const uintptr_t bases_UIMerchantItemMyShopWnd[] = {(uintptr_t)base_UIMerchantItemMyShopWnd, (uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMerchantItemMyShopWnd[] = {(uintptr_t)&type_UIMerchantItemMyShopWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMerchantItemMyShopWnd};
const uintptr_t locator_UIMerchantItemPurchaseMSGWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMerchantItemPurchaseMSGWnd, (uintptr_t)hierarchy_UIMerchantItemPurchaseMSGWnd};
const uintptr_t hierarchy_UIMerchantItemPurchaseMSGWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMerchantItemPurchaseMSGWnd};
const uintptr_t bases_UIMerchantItemPurchaseMSGWnd[] = {(uintptr_t)base_UIMerchantItemPurchaseMSGWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMerchantItemPurchaseMSGWnd[] = {(uintptr_t)&type_UIMerchantItemPurchaseMSGWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMerchantItemPurchaseMSGWnd};
const uintptr_t locator_UIMerchantItemPurchaseWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMerchantItemPurchaseWnd, (uintptr_t)hierarchy_UIMerchantItemPurchaseWnd};
const uintptr_t hierarchy_UIMerchantItemPurchaseWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIMerchantItemPurchaseWnd};
const uintptr_t bases_UIMerchantItemPurchaseWnd[] = {(uintptr_t)base_UIMerchantItemPurchaseWnd, (uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMerchantItemPurchaseWnd[] = {(uintptr_t)&type_UIMerchantItemPurchaseWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMerchantItemPurchaseWnd};
const uintptr_t locator_UINoticeMessageBox_LockSellItem[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINoticeMessageBox_LockSellItem, (uintptr_t)hierarchy_UINoticeMessageBox_LockSellItem};
const uintptr_t hierarchy_UINoticeMessageBox_LockSellItem[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UINoticeMessageBox_LockSellItem};
const uintptr_t bases_UINoticeMessageBox_LockSellItem[] = {(uintptr_t)base_UINoticeMessageBox_LockSellItem, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINoticeMessageBox_LockSellItem[] = {(uintptr_t)&type_UINoticeMessageBox_LockSellItem, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINoticeMessageBox_LockSellItem};
const uintptr_t locator_UIItemWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemWnd, (uintptr_t)hierarchy_UIItemWnd};
const uintptr_t hierarchy_UIItemWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIItemWnd};
const uintptr_t bases_UIItemWnd[] = {(uintptr_t)base_UIItemWnd, (uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemWnd[] = {(uintptr_t)&type_UIItemWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemWnd};
const uintptr_t locator_UIMerchantItemWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMerchantItemWnd, (uintptr_t)hierarchy_UIMerchantItemWnd};
const uintptr_t hierarchy_UIMerchantItemWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIMerchantItemWnd};
const uintptr_t bases_UIMerchantItemWnd[] = {(uintptr_t)base_UIMerchantItemWnd, (uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMerchantItemWnd[] = {(uintptr_t)&type_UIMerchantItemWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMerchantItemWnd};
const uintptr_t locator_UIMerchantMirrorItemWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMerchantMirrorItemWnd, (uintptr_t)hierarchy_UIMerchantMirrorItemWnd};
const uintptr_t hierarchy_UIMerchantMirrorItemWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIMerchantMirrorItemWnd};
const uintptr_t bases_UIMerchantMirrorItemWnd[] = {(uintptr_t)base_UIMerchantMirrorItemWnd, (uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMerchantMirrorItemWnd[] = {(uintptr_t)&type_UIMerchantMirrorItemWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMerchantMirrorItemWnd};
const uintptr_t locator_UIItemDropCntWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemDropCntWnd, (uintptr_t)hierarchy_UIItemDropCntWnd};
const uintptr_t locator_UIMerchantShopMakeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMerchantShopMakeWnd, (uintptr_t)hierarchy_UIMerchantShopMakeWnd};
const uintptr_t hierarchy_UIMerchantShopMakeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMerchantShopMakeWnd};
const uintptr_t bases_UIMerchantShopMakeWnd[] = {(uintptr_t)base_UIMerchantShopMakeWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMerchantShopMakeWnd[] = {(uintptr_t)&type_UIMerchantShopMakeWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMerchantShopMakeWnd};
const uintptr_t locator_UIMinimapZoomWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMinimapZoomWnd, (uintptr_t)hierarchy_UIMinimapZoomWnd};
const uintptr_t hierarchy_UIMinimapZoomWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMinimapZoomWnd};
const uintptr_t bases_UIMinimapZoomWnd[] = {(uintptr_t)base_UIMinimapZoomWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMinimapZoomWnd[] = {(uintptr_t)&type_UIMinimapZoomWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMinimapZoomWnd};
const uintptr_t locator_UINotifyLevelUpWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINotifyLevelUpWnd, (uintptr_t)hierarchy_UINotifyLevelUpWnd};
const uintptr_t hierarchy_UINotifyLevelUpWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UINotifyLevelUpWnd};
const uintptr_t bases_UINotifyLevelUpWnd[] = {(uintptr_t)base_UINotifyLevelUpWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINotifyLevelUpWnd[] = {(uintptr_t)&type_UINotifyLevelUpWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINotifyLevelUpWnd};
const uintptr_t locator_UINotifyJobLevelUpWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINotifyJobLevelUpWnd, (uintptr_t)hierarchy_UINotifyJobLevelUpWnd};
const uintptr_t hierarchy_UINotifyJobLevelUpWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UINotifyJobLevelUpWnd};
const uintptr_t bases_UINotifyJobLevelUpWnd[] = {(uintptr_t)base_UINotifyJobLevelUpWnd, (uintptr_t)base_UINotifyLevelUpWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINotifyJobLevelUpWnd[] = {(uintptr_t)&type_UINotifyJobLevelUpWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINotifyJobLevelUpWnd};
const uintptr_t locator_UINotifyQuestWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINotifyQuestWnd, (uintptr_t)hierarchy_UINotifyQuestWnd};
const uintptr_t hierarchy_UINotifyQuestWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UINotifyQuestWnd};
const uintptr_t bases_UINotifyQuestWnd[] = {(uintptr_t)base_UINotifyQuestWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINotifyQuestWnd[] = {(uintptr_t)&type_UINotifyQuestWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINotifyQuestWnd};
const uintptr_t locator_UInCash_CallWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UInCash_CallWnd, (uintptr_t)hierarchy_UInCash_CallWnd};
const uintptr_t hierarchy_UInCash_CallWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UInCash_CallWnd};
const uintptr_t bases_UInCash_CallWnd[] = {(uintptr_t)base_UInCash_CallWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UInCash_CallWnd[] = {(uintptr_t)&type_UInCash_CallWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UInCash_CallWnd};
const uintptr_t locator_UISecondPasswordEditWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISecondPasswordEditWnd, (uintptr_t)hierarchy_UISecondPasswordEditWnd};
const uintptr_t hierarchy_UISecondPasswordEditWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_UISecondPasswordEditWnd};
const uintptr_t bases_UISecondPasswordEditWnd[] = {(uintptr_t)base_UISecondPasswordEditWnd, (uintptr_t)base_UISecondPasswordInputWnd, (uintptr_t)base_UIStorePasswordInputWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISecondPasswordEditWnd[] = {(uintptr_t)&type_UISecondPasswordEditWnd, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISecondPasswordEditWnd};
const uintptr_t locator_UIEmotionWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEmotionWnd, (uintptr_t)hierarchy_UIEmotionWnd};
const uintptr_t hierarchy_UIEmotionWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIEmotionWnd};
const uintptr_t bases_UIEmotionWnd[] = {(uintptr_t)base_UIEmotionWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEmotionWnd[] = {(uintptr_t)&type_UIEmotionWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEmotionWnd};
const uintptr_t locator_CEffectPrim[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CEffectPrim, (uintptr_t)hierarchy_CEffectPrim};
const uintptr_t hierarchy_CEffectPrim[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_CEffectPrim};
const uintptr_t bases_CEffectPrim[] = {(uintptr_t)base_CEffectPrim, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CEffectPrim[] = {(uintptr_t)&type_CEffectPrim, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CEffectPrim};
const uintptr_t locator_CRagEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CRagEffect, (uintptr_t)hierarchy_CRagEffect};
const uintptr_t locator_CMousePointer[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CMousePointer, (uintptr_t)hierarchy_CMousePointer};
const uintptr_t hierarchy_CMousePointer[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_CMousePointer};
const uintptr_t bases_CMousePointer[] = {(uintptr_t)base_CMousePointer, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CMousePointer[] = {(uintptr_t)&type_CMousePointer, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CMousePointer};
const uintptr_t locator_CGameActor[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CGameActor, (uintptr_t)hierarchy_CGameActor};
const uintptr_t locator_CRenderObject[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CRenderObject, (uintptr_t)hierarchy_CRenderObject};
const uintptr_t locator_CAbleToMakeEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CAbleToMakeEffect, (uintptr_t)hierarchy_CAbleToMakeEffect};
const uintptr_t locator_C3dGrannyGameActor[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_C3dGrannyGameActor, (uintptr_t)hierarchy_C3dGrannyGameActor};
const uintptr_t hierarchy_C3dGrannyGameActor[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_C3dGrannyGameActor};
const uintptr_t bases_C3dGrannyGameActor[] = {(uintptr_t)base_C3dGrannyGameActor, (uintptr_t)base_CGameActor, (uintptr_t)base_CAbleToMakeEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_C3dGrannyGameActor[] = {(uintptr_t)&type_C3dGrannyGameActor, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_C3dGrannyGameActor};
const uintptr_t locator_UIVerticalNameBalloonText[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIVerticalNameBalloonText, (uintptr_t)hierarchy_UIVerticalNameBalloonText};
const uintptr_t hierarchy_UIVerticalNameBalloonText[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIVerticalNameBalloonText};
const uintptr_t bases_UIVerticalNameBalloonText[] = {(uintptr_t)base_UIVerticalNameBalloonText, (uintptr_t)base_UINameBalloonText, (uintptr_t)base_UIBalloonText, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIVerticalNameBalloonText[] = {(uintptr_t)&type_UIVerticalNameBalloonText, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIVerticalNameBalloonText};
const uintptr_t locator_CDummyEffectLauncher[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CDummyEffectLauncher, (uintptr_t)hierarchy_CDummyEffectLauncher};
const uintptr_t hierarchy_CDummyEffectLauncher[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_CDummyEffectLauncher};
const uintptr_t bases_CDummyEffectLauncher[] = {(uintptr_t)base_CDummyEffectLauncher, (uintptr_t)base_CAbleToMakeEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CDummyEffectLauncher[] = {(uintptr_t)&type_CDummyEffectLauncher, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CDummyEffectLauncher};
const uintptr_t locator_CItem[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CItem, (uintptr_t)hierarchy_CItem};
const uintptr_t hierarchy_CItem[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_CItem};
const uintptr_t bases_CItem[] = {(uintptr_t)base_CItem, (uintptr_t)base_CAbleToMakeEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CItem[] = {(uintptr_t)&type_CItem, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CItem};
const uintptr_t locator_CCorpse[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CCorpse, (uintptr_t)hierarchy_CCorpse};
const uintptr_t hierarchy_CCorpse[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_CCorpse};
const uintptr_t bases_CCorpse[] = {(uintptr_t)base_CCorpse, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CCorpse[] = {(uintptr_t)&type_CCorpse, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CCorpse};
const uintptr_t locator_CGndRiser[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CGndRiser, (uintptr_t)hierarchy_CGndRiser};
const uintptr_t hierarchy_CGndRiser[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_CGndRiser};
const uintptr_t bases_CGndRiser[] = {(uintptr_t)base_CGndRiser, (uintptr_t)base_CAbleToMakeEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CGndRiser[] = {(uintptr_t)&type_CGndRiser, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CGndRiser};
const uintptr_t locator_CSoundMaker[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CSoundMaker, (uintptr_t)hierarchy_CSoundMaker};
const uintptr_t hierarchy_CSoundMaker[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_CSoundMaker};
const uintptr_t bases_CSoundMaker[] = {(uintptr_t)base_CSoundMaker, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CSoundMaker[] = {(uintptr_t)&type_CSoundMaker, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CSoundMaker};
const uintptr_t locator_CNpc[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CNpc, (uintptr_t)hierarchy_CNpc};
const uintptr_t locator_CBlowEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CBlowEffect, (uintptr_t)hierarchy_CBlowEffect};
const uintptr_t hierarchy_CBlowEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_CBlowEffect};
const uintptr_t bases_CBlowEffect[] = {(uintptr_t)base_CBlowEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CBlowEffect[] = {(uintptr_t)&type_CBlowEffect, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CBlowEffect};
const uintptr_t locator_CBlurPC[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CBlurPC, (uintptr_t)hierarchy_CBlurPC};
const uintptr_t hierarchy_CBlurPC[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_CBlurPC};
const uintptr_t bases_CBlurPC[] = {(uintptr_t)base_CBlurPC, (uintptr_t)base_CPc, (uintptr_t)base_CGameActor, (uintptr_t)base_CAbleToMakeEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CBlurPC[] = {(uintptr_t)&type_CBlurPC, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CBlurPC};
const uintptr_t base_CPc[] = {(uintptr_t)&type_CPc, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CPc};
const uintptr_t hierarchy_CPc[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_CPc};
const uintptr_t bases_CPc[] = {(uintptr_t)base_CPc, (uintptr_t)base_CGameActor, (uintptr_t)base_CAbleToMakeEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t locator_CHalluPC[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CHalluPC, (uintptr_t)hierarchy_CHalluPC};
const uintptr_t hierarchy_CHalluPC[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_CHalluPC};
const uintptr_t bases_CHalluPC[] = {(uintptr_t)base_CHalluPC, (uintptr_t)base_CPc, (uintptr_t)base_CGameActor, (uintptr_t)base_CAbleToMakeEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CHalluPC[] = {(uintptr_t)&type_CHalluPC, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CHalluPC};
const uintptr_t locator_CHalluPC2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CHalluPC2, (uintptr_t)hierarchy_CHalluPC2};
const uintptr_t hierarchy_CHalluPC2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_CHalluPC2};
const uintptr_t bases_CHalluPC2[] = {(uintptr_t)base_CHalluPC2, (uintptr_t)base_CPc, (uintptr_t)base_CGameActor, (uintptr_t)base_CAbleToMakeEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CHalluPC2[] = {(uintptr_t)&type_CHalluPC2, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CHalluPC2};
const uintptr_t locator_CHalluNPC[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CHalluNPC, (uintptr_t)hierarchy_CHalluNPC};
const uintptr_t hierarchy_CHalluNPC[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_CHalluNPC};
const uintptr_t bases_CHalluNPC[] = {(uintptr_t)base_CHalluNPC, (uintptr_t)base_CNpc, (uintptr_t)base_CGameActor, (uintptr_t)base_CAbleToMakeEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CHalluNPC[] = {(uintptr_t)&type_CHalluNPC, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CHalluNPC};
const uintptr_t locator_CMasterChaser[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CMasterChaser, (uintptr_t)hierarchy_CMasterChaser};
const uintptr_t hierarchy_CMasterChaser[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_CMasterChaser};
const uintptr_t bases_CMasterChaser[] = {(uintptr_t)base_CMasterChaser, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CMasterChaser[] = {(uintptr_t)&type_CMasterChaser, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CMasterChaser};
const uintptr_t locator_CNumEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CNumEffect, (uintptr_t)hierarchy_CNumEffect};
const uintptr_t hierarchy_CNumEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_CNumEffect};
const uintptr_t bases_CNumEffect[] = {(uintptr_t)base_CNumEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CNumEffect[] = {(uintptr_t)&type_CNumEffect, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CNumEffect};
const uintptr_t locator_CHalluPlant[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CHalluPlant, (uintptr_t)hierarchy_CHalluPlant};
const uintptr_t hierarchy_CHalluPlant[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_CHalluPlant};
const uintptr_t bases_CHalluPlant[] = {(uintptr_t)base_CHalluPlant, (uintptr_t)base_CNpc, (uintptr_t)base_CGameActor, (uintptr_t)base_CAbleToMakeEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CHalluPlant[] = {(uintptr_t)&type_CHalluPlant, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CHalluPlant};
const uintptr_t locator_CPc[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CPc, (uintptr_t)hierarchy_CPc};
const uintptr_t locator_CGrannyPc[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CGrannyPc, (uintptr_t)hierarchy_CGrannyPc};
const uintptr_t hierarchy_CGrannyPc[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_CGrannyPc};
const uintptr_t bases_CGrannyPc[] = {(uintptr_t)base_CGrannyPc, (uintptr_t)base_CGameActor, (uintptr_t)base_CAbleToMakeEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CGrannyPc[] = {(uintptr_t)&type_CGrannyPc, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CGrannyPc};
const uintptr_t locator_CPlayer[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CPlayer, (uintptr_t)hierarchy_CPlayer};
const uintptr_t hierarchy_CPlayer[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_CPlayer};
const uintptr_t bases_CPlayer[] = {(uintptr_t)base_CPlayer, (uintptr_t)base_CPc, (uintptr_t)base_CGameActor, (uintptr_t)base_CAbleToMakeEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CPlayer[] = {(uintptr_t)&type_CPlayer, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CPlayer};
const uintptr_t locator_CMsgEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CMsgEffect, (uintptr_t)hierarchy_CMsgEffect};
const uintptr_t hierarchy_CMsgEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_CMsgEffect};
const uintptr_t bases_CMsgEffect[] = {(uintptr_t)base_CMsgEffect, (uintptr_t)base_CAbleToMakeEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CMsgEffect[] = {(uintptr_t)&type_CMsgEffect, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CMsgEffect};
const uintptr_t locator_CGroundScope[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CGroundScope, (uintptr_t)hierarchy_CGroundScope};
const uintptr_t hierarchy_CGroundScope[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_CGroundScope};
const uintptr_t bases_CGroundScope[] = {(uintptr_t)base_CGroundScope, (uintptr_t)base_CAbleToMakeEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CGroundScope[] = {(uintptr_t)&type_CGroundScope, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CGroundScope};
const uintptr_t locator_CEffectMaker[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CEffectMaker, (uintptr_t)hierarchy_CEffectMaker};
const uintptr_t hierarchy_CEffectMaker[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_CEffectMaker};
const uintptr_t bases_CEffectMaker[] = {(uintptr_t)base_CEffectMaker, (uintptr_t)base_CAbleToMakeEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CEffectMaker[] = {(uintptr_t)&type_CEffectMaker, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CEffectMaker};
const uintptr_t locator_CSkill[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CSkill, (uintptr_t)hierarchy_CSkill};
const uintptr_t hierarchy_CSkill[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_CSkill};
const uintptr_t bases_CSkill[] = {(uintptr_t)base_CSkill, (uintptr_t)base_CGameActor, (uintptr_t)base_CAbleToMakeEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CSkill[] = {(uintptr_t)&type_CSkill, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CSkill};
const uintptr_t locator_CGraffiSkill[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CGraffiSkill, (uintptr_t)hierarchy_CGraffiSkill};
const uintptr_t hierarchy_CGraffiSkill[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_CGraffiSkill};
const uintptr_t bases_CGraffiSkill[] = {(uintptr_t)base_CGraffiSkill, (uintptr_t)base_CSkill, (uintptr_t)base_CGameActor, (uintptr_t)base_CAbleToMakeEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CGraffiSkill[] = {(uintptr_t)&type_CGraffiSkill, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CGraffiSkill};
const uintptr_t locator_CArrowEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CArrowEffect, (uintptr_t)hierarchy_CArrowEffect};
const uintptr_t hierarchy_CArrowEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_CArrowEffect};
const uintptr_t bases_CArrowEffect[] = {(uintptr_t)base_CArrowEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CArrowEffect[] = {(uintptr_t)&type_CArrowEffect, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CArrowEffect};
const uintptr_t locator_CMandraAttackEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CMandraAttackEffect, (uintptr_t)hierarchy_CMandraAttackEffect};
const uintptr_t hierarchy_CMandraAttackEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_CMandraAttackEffect};
const uintptr_t bases_CMandraAttackEffect[] = {(uintptr_t)base_CMandraAttackEffect, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CMandraAttackEffect[] = {(uintptr_t)&type_CMandraAttackEffect, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CMandraAttackEffect};
const uintptr_t locator_UIEnchant_Tab[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEnchant_Tab, (uintptr_t)hierarchy_UIEnchant_Tab};
const uintptr_t hierarchy_UIEnchant_Tab[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIEnchant_Tab};
const uintptr_t bases_UIEnchant_Tab[] = {(uintptr_t)base_UIEnchant_Tab, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEnchant_Tab[] = {(uintptr_t)&type_UIEnchant_Tab, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEnchant_Tab};
const uintptr_t locator_UIEnchant_Group[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEnchant_Group, (uintptr_t)hierarchy_UIEnchant_Group};
const uintptr_t hierarchy_UIEnchant_Group[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIEnchant_Group};
const uintptr_t bases_UIEnchant_Group[] = {(uintptr_t)base_UIEnchant_Group, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEnchant_Group[] = {(uintptr_t)&type_UIEnchant_Group, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEnchant_Group};
const uintptr_t locator_UIEnchant_ScrollBar[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEnchant_ScrollBar, (uintptr_t)hierarchy_UIEnchant_ScrollBar};
const uintptr_t hierarchy_UIEnchant_ScrollBar[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIEnchant_ScrollBar};
const uintptr_t bases_UIEnchant_ScrollBar[] = {(uintptr_t)base_UIEnchant_ScrollBar, (uintptr_t)base_UIScrollBar, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEnchant_ScrollBar[] = {(uintptr_t)&type_UIEnchant_ScrollBar, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEnchant_ScrollBar};
const uintptr_t locator_UIEnchant_List[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEnchant_List, (uintptr_t)hierarchy_UIEnchant_List};
const uintptr_t hierarchy_UIEnchant_List[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIEnchant_List};
const uintptr_t bases_UIEnchant_List[] = {(uintptr_t)base_UIEnchant_List, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEnchant_List[] = {(uintptr_t)&type_UIEnchant_List, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEnchant_List};
const uintptr_t locator_UIEnchant_TargetList[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEnchant_TargetList, (uintptr_t)hierarchy_UIEnchant_TargetList};
const uintptr_t hierarchy_UIEnchant_TargetList[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIEnchant_TargetList};
const uintptr_t bases_UIEnchant_TargetList[] = {(uintptr_t)base_UIEnchant_TargetList, (uintptr_t)base_UIEnchant_List, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEnchant_TargetList[] = {(uintptr_t)&type_UIEnchant_TargetList, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEnchant_TargetList};
const uintptr_t locator_UIEnchant_Item[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEnchant_Item, (uintptr_t)hierarchy_UIEnchant_Item};
const uintptr_t hierarchy_UIEnchant_Item[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIEnchant_Item};
const uintptr_t bases_UIEnchant_Item[] = {(uintptr_t)base_UIEnchant_Item, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEnchant_Item[] = {(uintptr_t)&type_UIEnchant_Item, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEnchant_Item};
const uintptr_t locator_UIEnchant_Button[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEnchant_Button, (uintptr_t)hierarchy_UIEnchant_Button};
const uintptr_t hierarchy_UIEnchant_Button[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIEnchant_Button};
const uintptr_t bases_UIEnchant_Button[] = {(uintptr_t)base_UIEnchant_Button, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEnchant_Button[] = {(uintptr_t)&type_UIEnchant_Button, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEnchant_Button};
const uintptr_t locator_UIEnchant_Info[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEnchant_Info, (uintptr_t)hierarchy_UIEnchant_Info};
const uintptr_t hierarchy_UIEnchant_Info[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIEnchant_Info};
const uintptr_t bases_UIEnchant_Info[] = {(uintptr_t)base_UIEnchant_Info, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEnchant_Info[] = {(uintptr_t)&type_UIEnchant_Info, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEnchant_Info};
const uintptr_t locator_UIEnchantWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEnchantWnd, (uintptr_t)hierarchy_UIEnchantWnd};
const uintptr_t hierarchy_UIEnchantWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIEnchantWnd};
const uintptr_t bases_UIEnchantWnd[] = {(uintptr_t)base_UIEnchantWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEnchantWnd[] = {(uintptr_t)&type_UIEnchantWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEnchantWnd};
const uintptr_t locator_UIItemReformWnd_List[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemReformWnd_List, (uintptr_t)hierarchy_UIItemReformWnd_List};
const uintptr_t hierarchy_UIItemReformWnd_List[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIItemReformWnd_List};
const uintptr_t bases_UIItemReformWnd_List[] = {(uintptr_t)base_UIItemReformWnd_List, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemReformWnd_List[] = {(uintptr_t)&type_UIItemReformWnd_List, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemReformWnd_List};
const uintptr_t locator_UIItemReformWnd_Item[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemReformWnd_Item, (uintptr_t)hierarchy_UIItemReformWnd_Item};
const uintptr_t hierarchy_UIItemReformWnd_Item[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIItemReformWnd_Item};
const uintptr_t bases_UIItemReformWnd_Item[] = {(uintptr_t)base_UIItemReformWnd_Item, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemReformWnd_Item[] = {(uintptr_t)&type_UIItemReformWnd_Item, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemReformWnd_Item};
const uintptr_t locator_UIItemReformWnd_DetailBtn[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemReformWnd_DetailBtn, (uintptr_t)hierarchy_UIItemReformWnd_DetailBtn};
const uintptr_t hierarchy_UIItemReformWnd_DetailBtn[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIItemReformWnd_DetailBtn};
const uintptr_t bases_UIItemReformWnd_DetailBtn[] = {(uintptr_t)base_UIItemReformWnd_DetailBtn, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemReformWnd_DetailBtn[] = {(uintptr_t)&type_UIItemReformWnd_DetailBtn, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemReformWnd_DetailBtn};
const uintptr_t locator_UIItemReformWnd_DetailMsg[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemReformWnd_DetailMsg, (uintptr_t)hierarchy_UIItemReformWnd_DetailMsg};
const uintptr_t hierarchy_UIItemReformWnd_DetailMsg[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIItemReformWnd_DetailMsg};
const uintptr_t bases_UIItemReformWnd_DetailMsg[] = {(uintptr_t)base_UIItemReformWnd_DetailMsg, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemReformWnd_DetailMsg[] = {(uintptr_t)&type_UIItemReformWnd_DetailMsg, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemReformWnd_DetailMsg};
const uintptr_t locator_UIItemReformWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemReformWnd, (uintptr_t)hierarchy_UIItemReformWnd};
const uintptr_t hierarchy_UIItemReformWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIItemReformWnd};
const uintptr_t bases_UIItemReformWnd[] = {(uintptr_t)base_UIItemReformWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemReformWnd[] = {(uintptr_t)&type_UIItemReformWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemReformWnd};
const uintptr_t locator_UILapineScrollBar[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UILapineScrollBar, (uintptr_t)hierarchy_UILapineScrollBar};
const uintptr_t hierarchy_UILapineScrollBar[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UILapineScrollBar};
const uintptr_t bases_UILapineScrollBar[] = {(uintptr_t)base_UILapineScrollBar, (uintptr_t)base_UIScrollBar, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UILapineScrollBar[] = {(uintptr_t)&type_UILapineScrollBar, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UILapineScrollBar};
const uintptr_t locator_UIIngredientInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIIngredientInfoWnd, (uintptr_t)hierarchy_UIIngredientInfoWnd};
const uintptr_t hierarchy_UIIngredientInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIIngredientInfoWnd};
const uintptr_t bases_UIIngredientInfoWnd[] = {(uintptr_t)base_UIIngredientInfoWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIIngredientInfoWnd[] = {(uintptr_t)&type_UIIngredientInfoWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIIngredientInfoWnd};
const uintptr_t locator_UILapineBox_IngredientListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UILapineBox_IngredientListWnd, (uintptr_t)hierarchy_UILapineBox_IngredientListWnd};
const uintptr_t hierarchy_UILapineBox_IngredientListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UILapineBox_IngredientListWnd};
const uintptr_t bases_UILapineBox_IngredientListWnd[] = {(uintptr_t)base_UILapineBox_IngredientListWnd, (uintptr_t)base_UIScrollWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UILapineBox_IngredientListWnd[] = {(uintptr_t)&type_UILapineBox_IngredientListWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UILapineBox_IngredientListWnd};
const uintptr_t base_UIScrollWnd[] = {(uintptr_t)&type_UIScrollWnd, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIScrollWnd};
const uintptr_t hierarchy_UIScrollWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIScrollWnd};
const uintptr_t bases_UIScrollWnd[] = {(uintptr_t)base_UIScrollWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t locator_UILapineBoxWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UILapineBoxWnd, (uintptr_t)hierarchy_UILapineBoxWnd};
const uintptr_t hierarchy_UILapineBoxWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UILapineBoxWnd};
const uintptr_t bases_UILapineBoxWnd[] = {(uintptr_t)base_UILapineBoxWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UILapineBoxWnd[] = {(uintptr_t)&type_UILapineBoxWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UILapineBoxWnd};
const uintptr_t locator_UILapineUpgradeBox_IngredientInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UILapineUpgradeBox_IngredientInfoWnd, (uintptr_t)hierarchy_UILapineUpgradeBox_IngredientInfoWnd};
const uintptr_t hierarchy_UILapineUpgradeBox_IngredientInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UILapineUpgradeBox_IngredientInfoWnd};
const uintptr_t bases_UILapineUpgradeBox_IngredientInfoWnd[] = {(uintptr_t)base_UILapineUpgradeBox_IngredientInfoWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UILapineUpgradeBox_IngredientInfoWnd[] = {(uintptr_t)&type_UILapineUpgradeBox_IngredientInfoWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UILapineUpgradeBox_IngredientInfoWnd};
const uintptr_t locator_UILapineUpgradeBox_IngredientListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UILapineUpgradeBox_IngredientListWnd, (uintptr_t)hierarchy_UILapineUpgradeBox_IngredientListWnd};
const uintptr_t hierarchy_UILapineUpgradeBox_IngredientListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UILapineUpgradeBox_IngredientListWnd};
const uintptr_t bases_UILapineUpgradeBox_IngredientListWnd[] = {(uintptr_t)base_UILapineUpgradeBox_IngredientListWnd, (uintptr_t)base_UIScrollWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UILapineUpgradeBox_IngredientListWnd[] = {(uintptr_t)&type_UILapineUpgradeBox_IngredientListWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UILapineUpgradeBox_IngredientListWnd};
const uintptr_t locator_UILapineUpgradeBoxWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UILapineUpgradeBoxWnd, (uintptr_t)hierarchy_UILapineUpgradeBoxWnd};
const uintptr_t hierarchy_UILapineUpgradeBoxWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UILapineUpgradeBoxWnd};
const uintptr_t bases_UILapineUpgradeBoxWnd[] = {(uintptr_t)base_UILapineUpgradeBoxWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UILapineUpgradeBoxWnd[] = {(uintptr_t)&type_UILapineUpgradeBoxWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UILapineUpgradeBoxWnd};
const uintptr_t locator_UIReputeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIReputeWnd, (uintptr_t)hierarchy_UIReputeWnd};
const uintptr_t hierarchy_UIReputeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIReputeWnd};
const uintptr_t bases_UIReputeWnd[] = {(uintptr_t)base_UIReputeWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIReputeWnd[] = {(uintptr_t)&type_UIReputeWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIReputeWnd};
const uintptr_t locator_CAcidifiedThrow[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CAcidifiedThrow, (uintptr_t)hierarchy_CAcidifiedThrow};
const uintptr_t hierarchy_CAcidifiedThrow[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CAcidifiedThrow};
const uintptr_t bases_CAcidifiedThrow[] = {(uintptr_t)base_CAcidifiedThrow, (uintptr_t)base_CEffect};
const uintptr_t base_CAcidifiedThrow[] = {(uintptr_t)&type_CAcidifiedThrow, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CAcidifiedThrow};
const uintptr_t base_CEffect[] = {(uintptr_t)&type_CEffect, (uintptr_t)0, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CEffect};
const uintptr_t hierarchy_CEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)1, (uintptr_t)bases_CEffect};
const uintptr_t bases_CEffect[] = {(uintptr_t)base_CEffect};
const uintptr_t locator_CAxeBoomerang[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CAxeBoomerang, (uintptr_t)hierarchy_CAxeBoomerang};
const uintptr_t hierarchy_CAxeBoomerang[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CAxeBoomerang};
const uintptr_t bases_CAxeBoomerang[] = {(uintptr_t)base_CAxeBoomerang, (uintptr_t)base_CEffect};
const uintptr_t base_CAxeBoomerang[] = {(uintptr_t)&type_CAxeBoomerang, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CAxeBoomerang};
const uintptr_t locator_CAnimatedEmitterEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CAnimatedEmitterEffect, (uintptr_t)hierarchy_CAnimatedEmitterEffect};
const uintptr_t hierarchy_CAnimatedEmitterEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_CAnimatedEmitterEffect};
const uintptr_t bases_CAnimatedEmitterEffect[] = {(uintptr_t)base_CAnimatedEmitterEffect, (uintptr_t)base_CEmitterEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CAnimatedEmitterEffect[] = {(uintptr_t)&type_CAnimatedEmitterEffect, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CAnimatedEmitterEffect};
const uintptr_t base_CEmitterEffect[] = {(uintptr_t)&type_CEmitterEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CEmitterEffect};
const uintptr_t hierarchy_CEmitterEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CEmitterEffect};
const uintptr_t bases_CEmitterEffect[] = {(uintptr_t)base_CEmitterEffect, (uintptr_t)base_CEffect};
const uintptr_t locator_CCartCannon[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CCartCannon, (uintptr_t)hierarchy_CCartCannon};
const uintptr_t hierarchy_CCartCannon[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CCartCannon};
const uintptr_t bases_CCartCannon[] = {(uintptr_t)base_CCartCannon, (uintptr_t)base_CEffect};
const uintptr_t base_CCartCannon[] = {(uintptr_t)&type_CCartCannon, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CCartCannon};
const uintptr_t locator_CBindTrapEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CBindTrapEffect, (uintptr_t)hierarchy_CBindTrapEffect};
const uintptr_t hierarchy_CBindTrapEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CBindTrapEffect};
const uintptr_t bases_CBindTrapEffect[] = {(uintptr_t)base_CBindTrapEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CBindTrapEffect[] = {(uintptr_t)&type_CBindTrapEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CBindTrapEffect};
const uintptr_t locator_CCloud[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CCloud, (uintptr_t)hierarchy_CCloud};
const uintptr_t hierarchy_CCloud[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CCloud};
const uintptr_t bases_CCloud[] = {(uintptr_t)base_CCloud, (uintptr_t)base_CEffect};
const uintptr_t base_CCloud[] = {(uintptr_t)&type_CCloud, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CCloud};
const uintptr_t locator_CCloudKillEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CCloudKillEffect, (uintptr_t)hierarchy_CCloudKillEffect};
const uintptr_t hierarchy_CCloudKillEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CCloudKillEffect};
const uintptr_t bases_CCloudKillEffect[] = {(uintptr_t)base_CCloudKillEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CCloudKillEffect[] = {(uintptr_t)&type_CCloudKillEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CCloudKillEffect};
const uintptr_t locator_CCommonFloorEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CCommonFloorEffect, (uintptr_t)hierarchy_CCommonFloorEffect};
const uintptr_t hierarchy_CCommonFloorEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CCommonFloorEffect};
const uintptr_t bases_CCommonFloorEffect[] = {(uintptr_t)base_CCommonFloorEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CCommonFloorEffect[] = {(uintptr_t)&type_CCommonFloorEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CCommonFloorEffect};
const uintptr_t locator_CEvilLandEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CEvilLandEffect, (uintptr_t)hierarchy_CEvilLandEffect};
const uintptr_t hierarchy_CEvilLandEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CEvilLandEffect};
const uintptr_t bases_CEvilLandEffect[] = {(uintptr_t)base_CEvilLandEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CEvilLandEffect[] = {(uintptr_t)&type_CEvilLandEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CEvilLandEffect};
const uintptr_t locator_CEvilsPawEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CEvilsPawEffect, (uintptr_t)hierarchy_CEvilsPawEffect};
const uintptr_t hierarchy_CEvilsPawEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CEvilsPawEffect};
const uintptr_t bases_CEvilsPawEffect[] = {(uintptr_t)base_CEvilsPawEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CEvilsPawEffect[] = {(uintptr_t)&type_CEvilsPawEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CEvilsPawEffect};
const uintptr_t locator_CFortuneKiss[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CFortuneKiss, (uintptr_t)hierarchy_CFortuneKiss};
const uintptr_t hierarchy_CFortuneKiss[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CFortuneKiss};
const uintptr_t bases_CFortuneKiss[] = {(uintptr_t)base_CFortuneKiss, (uintptr_t)base_CEffect};
const uintptr_t base_CFortuneKiss[] = {(uintptr_t)&type_CFortuneKiss, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CFortuneKiss};
const uintptr_t locator_CFriggSongEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CFriggSongEffect, (uintptr_t)hierarchy_CFriggSongEffect};
const uintptr_t hierarchy_CFriggSongEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CFriggSongEffect};
const uintptr_t bases_CFriggSongEffect[] = {(uintptr_t)base_CFriggSongEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CFriggSongEffect[] = {(uintptr_t)&type_CFriggSongEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CFriggSongEffect};
const uintptr_t locator_CChangeSizeEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CChangeSizeEffect, (uintptr_t)hierarchy_CChangeSizeEffect};
const uintptr_t hierarchy_CChangeSizeEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CChangeSizeEffect};
const uintptr_t bases_CChangeSizeEffect[] = {(uintptr_t)base_CChangeSizeEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CChangeSizeEffect[] = {(uintptr_t)&type_CChangeSizeEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CChangeSizeEffect};
const uintptr_t locator_CChargingPierceEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CChargingPierceEffect, (uintptr_t)hierarchy_CChargingPierceEffect};
const uintptr_t hierarchy_CChargingPierceEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CChargingPierceEffect};
const uintptr_t bases_CChargingPierceEffect[] = {(uintptr_t)base_CChargingPierceEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CChargingPierceEffect[] = {(uintptr_t)&type_CChargingPierceEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CChargingPierceEffect};
const uintptr_t locator_CHitLineEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CHitLineEffect, (uintptr_t)hierarchy_CHitLineEffect};
const uintptr_t hierarchy_CHitLineEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CHitLineEffect};
const uintptr_t bases_CHitLineEffect[] = {(uintptr_t)base_CHitLineEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CHitLineEffect[] = {(uintptr_t)&type_CHitLineEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CHitLineEffect};
const uintptr_t locator_CMeteorEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CMeteorEffect, (uintptr_t)hierarchy_CMeteorEffect};
const uintptr_t hierarchy_CMeteorEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CMeteorEffect};
const uintptr_t bases_CMeteorEffect[] = {(uintptr_t)base_CMeteorEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CMeteorEffect[] = {(uintptr_t)&type_CMeteorEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CMeteorEffect};
const uintptr_t locator_CNyanggrassEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CNyanggrassEffect, (uintptr_t)hierarchy_CNyanggrassEffect};
const uintptr_t hierarchy_CNyanggrassEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CNyanggrassEffect};
const uintptr_t bases_CNyanggrassEffect[] = {(uintptr_t)base_CNyanggrassEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CNyanggrassEffect[] = {(uintptr_t)&type_CNyanggrassEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CNyanggrassEffect};
const uintptr_t locator_CPowderingEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CPowderingEffect, (uintptr_t)hierarchy_CPowderingEffect};
const uintptr_t hierarchy_CPowderingEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CPowderingEffect};
const uintptr_t bases_CPowderingEffect[] = {(uintptr_t)base_CPowderingEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CPowderingEffect[] = {(uintptr_t)&type_CPowderingEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CPowderingEffect};
const uintptr_t locator_CRectUpEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CRectUpEffect, (uintptr_t)hierarchy_CRectUpEffect};
const uintptr_t hierarchy_CRectUpEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CRectUpEffect};
const uintptr_t bases_CRectUpEffect[] = {(uintptr_t)base_CRectUpEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CRectUpEffect[] = {(uintptr_t)&type_CRectUpEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CRectUpEffect};
const uintptr_t locator_CRichManKim[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CRichManKim, (uintptr_t)hierarchy_CRichManKim};
const uintptr_t hierarchy_CRichManKim[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CRichManKim};
const uintptr_t bases_CRichManKim[] = {(uintptr_t)base_CRichManKim, (uintptr_t)base_CEffect};
const uintptr_t base_CRichManKim[] = {(uintptr_t)&type_CRichManKim, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CRichManKim};
const uintptr_t locator_CRootTwistEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CRootTwistEffect, (uintptr_t)hierarchy_CRootTwistEffect};
const uintptr_t hierarchy_CRootTwistEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CRootTwistEffect};
const uintptr_t bases_CRootTwistEffect[] = {(uintptr_t)base_CRootTwistEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CRootTwistEffect[] = {(uintptr_t)&type_CRootTwistEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CRootTwistEffect};
const uintptr_t locator_CSpriteMableEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CSpriteMableEffect, (uintptr_t)hierarchy_CSpriteMableEffect};
const uintptr_t hierarchy_CSpriteMableEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CSpriteMableEffect};
const uintptr_t bases_CSpriteMableEffect[] = {(uintptr_t)base_CSpriteMableEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CSpriteMableEffect[] = {(uintptr_t)&type_CSpriteMableEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CSpriteMableEffect};
const uintptr_t locator_CStemSpearEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CStemSpearEffect, (uintptr_t)hierarchy_CStemSpearEffect};
const uintptr_t hierarchy_CStemSpearEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CStemSpearEffect};
const uintptr_t bases_CStemSpearEffect[] = {(uintptr_t)base_CStemSpearEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CStemSpearEffect[] = {(uintptr_t)&type_CStemSpearEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CStemSpearEffect};
const uintptr_t locator_CSvgSpiritEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CSvgSpiritEffect, (uintptr_t)hierarchy_CSvgSpiritEffect};
const uintptr_t hierarchy_CSvgSpiritEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CSvgSpiritEffect};
const uintptr_t bases_CSvgSpiritEffect[] = {(uintptr_t)base_CSvgSpiritEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CSvgSpiritEffect[] = {(uintptr_t)&type_CSvgSpiritEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CSvgSpiritEffect};
const uintptr_t locator_CTunaPartyEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CTunaPartyEffect, (uintptr_t)hierarchy_CTunaPartyEffect};
const uintptr_t hierarchy_CTunaPartyEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CTunaPartyEffect};
const uintptr_t bases_CTunaPartyEffect[] = {(uintptr_t)base_CTunaPartyEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CTunaPartyEffect[] = {(uintptr_t)&type_CTunaPartyEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CTunaPartyEffect};
const uintptr_t locator_CWarningPlaneEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CWarningPlaneEffect, (uintptr_t)hierarchy_CWarningPlaneEffect};
const uintptr_t hierarchy_CWarningPlaneEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CWarningPlaneEffect};
const uintptr_t bases_CWarningPlaneEffect[] = {(uintptr_t)base_CWarningPlaneEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CWarningPlaneEffect[] = {(uintptr_t)&type_CWarningPlaneEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CWarningPlaneEffect};
const uintptr_t locator_CFallenAngelEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CFallenAngelEffect, (uintptr_t)hierarchy_CFallenAngelEffect};
const uintptr_t hierarchy_CFallenAngelEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CFallenAngelEffect};
const uintptr_t bases_CFallenAngelEffect[] = {(uintptr_t)base_CFallenAngelEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CFallenAngelEffect[] = {(uintptr_t)&type_CFallenAngelEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CFallenAngelEffect};
const uintptr_t locator_CGravityControlEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CGravityControlEffect, (uintptr_t)hierarchy_CGravityControlEffect};
const uintptr_t hierarchy_CGravityControlEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CGravityControlEffect};
const uintptr_t bases_CGravityControlEffect[] = {(uintptr_t)base_CGravityControlEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CGravityControlEffect[] = {(uintptr_t)&type_CGravityControlEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CGravityControlEffect};
const uintptr_t locator_CJupitelThunderHit[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CJupitelThunderHit, (uintptr_t)hierarchy_CJupitelThunderHit};
const uintptr_t hierarchy_CJupitelThunderHit[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CJupitelThunderHit};
const uintptr_t bases_CJupitelThunderHit[] = {(uintptr_t)base_CJupitelThunderHit, (uintptr_t)base_CEffect};
const uintptr_t base_CJupitelThunderHit[] = {(uintptr_t)&type_CJupitelThunderHit, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CJupitelThunderHit};
const uintptr_t locator_CNewCloudKillEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CNewCloudKillEffect, (uintptr_t)hierarchy_CNewCloudKillEffect};
const uintptr_t hierarchy_CNewCloudKillEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CNewCloudKillEffect};
const uintptr_t bases_CNewCloudKillEffect[] = {(uintptr_t)base_CNewCloudKillEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CNewCloudKillEffect[] = {(uintptr_t)&type_CNewCloudKillEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CNewCloudKillEffect};
const uintptr_t locator_CRandomOptEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CRandomOptEffect, (uintptr_t)hierarchy_CRandomOptEffect};
const uintptr_t hierarchy_CRandomOptEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CRandomOptEffect};
const uintptr_t bases_CRandomOptEffect[] = {(uintptr_t)base_CRandomOptEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CRandomOptEffect[] = {(uintptr_t)&type_CRandomOptEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CRandomOptEffect};
const uintptr_t locator_CRichsCoinEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CRichsCoinEffect, (uintptr_t)hierarchy_CRichsCoinEffect};
const uintptr_t hierarchy_CRichsCoinEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CRichsCoinEffect};
const uintptr_t bases_CRichsCoinEffect[] = {(uintptr_t)base_CRichsCoinEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CRichsCoinEffect[] = {(uintptr_t)&type_CRichsCoinEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CRichsCoinEffect};
const uintptr_t locator_CServantWeaponEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CServantWeaponEffect, (uintptr_t)hierarchy_CServantWeaponEffect};
const uintptr_t hierarchy_CServantWeaponEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CServantWeaponEffect};
const uintptr_t bases_CServantWeaponEffect[] = {(uintptr_t)base_CServantWeaponEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CServantWeaponEffect[] = {(uintptr_t)&type_CServantWeaponEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CServantWeaponEffect};
const uintptr_t locator_CSevereRainStorm[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CSevereRainStorm, (uintptr_t)hierarchy_CSevereRainStorm};
const uintptr_t hierarchy_CSevereRainStorm[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CSevereRainStorm};
const uintptr_t bases_CSevereRainStorm[] = {(uintptr_t)base_CSevereRainStorm, (uintptr_t)base_CEffect};
const uintptr_t base_CSevereRainStorm[] = {(uintptr_t)&type_CSevereRainStorm, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CSevereRainStorm};
const uintptr_t locator_CShowEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CShowEffect, (uintptr_t)hierarchy_CShowEffect};
const uintptr_t hierarchy_CShowEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CShowEffect};
const uintptr_t bases_CShowEffect[] = {(uintptr_t)base_CShowEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CShowEffect[] = {(uintptr_t)&type_CShowEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CShowEffect};
const uintptr_t locator_CSoulCollectEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CSoulCollectEffect, (uintptr_t)hierarchy_CSoulCollectEffect};
const uintptr_t hierarchy_CSoulCollectEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CSoulCollectEffect};
const uintptr_t bases_CSoulCollectEffect[] = {(uintptr_t)base_CSoulCollectEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CSoulCollectEffect[] = {(uintptr_t)&type_CSoulCollectEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CSoulCollectEffect};
const uintptr_t locator_CSoulUnityEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CSoulUnityEffect, (uintptr_t)hierarchy_CSoulUnityEffect};
const uintptr_t hierarchy_CSoulUnityEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CSoulUnityEffect};
const uintptr_t bases_CSoulUnityEffect[] = {(uintptr_t)base_CSoulUnityEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CSoulUnityEffect[] = {(uintptr_t)&type_CSoulUnityEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CSoulUnityEffect};
const uintptr_t locator_CStarEmperorEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CStarEmperorEffect, (uintptr_t)hierarchy_CStarEmperorEffect};
const uintptr_t hierarchy_CStarEmperorEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CStarEmperorEffect};
const uintptr_t bases_CStarEmperorEffect[] = {(uintptr_t)base_CStarEmperorEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CStarEmperorEffect[] = {(uintptr_t)&type_CStarEmperorEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CStarEmperorEffect};
const uintptr_t locator_CSunStanceEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CSunStanceEffect, (uintptr_t)hierarchy_CSunStanceEffect};
const uintptr_t hierarchy_CSunStanceEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CSunStanceEffect};
const uintptr_t bases_CSunStanceEffect[] = {(uintptr_t)base_CSunStanceEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CSunStanceEffect[] = {(uintptr_t)&type_CSunStanceEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CSunStanceEffect};
const uintptr_t locator_CTetraBallEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CTetraBallEffect, (uintptr_t)hierarchy_CTetraBallEffect};
const uintptr_t hierarchy_CTetraBallEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CTetraBallEffect};
const uintptr_t bases_CTetraBallEffect[] = {(uintptr_t)base_CTetraBallEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CTetraBallEffect[] = {(uintptr_t)&type_CTetraBallEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CTetraBallEffect};
const uintptr_t locator_CTriangleShotArrow[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CTriangleShotArrow, (uintptr_t)hierarchy_CTriangleShotArrow};
const uintptr_t hierarchy_CTriangleShotArrow[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CTriangleShotArrow};
const uintptr_t bases_CTriangleShotArrow[] = {(uintptr_t)base_CTriangleShotArrow, (uintptr_t)base_CEffect};
const uintptr_t base_CTriangleShotArrow[] = {(uintptr_t)&type_CTriangleShotArrow, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CTriangleShotArrow};
const uintptr_t locator_CBash[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CBash, (uintptr_t)hierarchy_CBash};
const uintptr_t hierarchy_CBash[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CBash};
const uintptr_t bases_CBash[] = {(uintptr_t)base_CBash, (uintptr_t)base_CEffect};
const uintptr_t base_CBash[] = {(uintptr_t)&type_CBash, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CBash};
const uintptr_t locator_CCellRangeEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CCellRangeEffect, (uintptr_t)hierarchy_CCellRangeEffect};
const uintptr_t hierarchy_CCellRangeEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CCellRangeEffect};
const uintptr_t bases_CCellRangeEffect[] = {(uintptr_t)base_CCellRangeEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CCellRangeEffect[] = {(uintptr_t)&type_CCellRangeEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CCellRangeEffect};
const uintptr_t locator_CCherryBlossomEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CCherryBlossomEffect, (uintptr_t)hierarchy_CCherryBlossomEffect};
const uintptr_t hierarchy_CCherryBlossomEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CCherryBlossomEffect};
const uintptr_t bases_CCherryBlossomEffect[] = {(uintptr_t)base_CCherryBlossomEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CCherryBlossomEffect[] = {(uintptr_t)&type_CCherryBlossomEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CCherryBlossomEffect};
const uintptr_t locator_CGlassWall[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CGlassWall, (uintptr_t)hierarchy_CGlassWall};
const uintptr_t hierarchy_CGlassWall[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CGlassWall};
const uintptr_t bases_CGlassWall[] = {(uintptr_t)base_CGlassWall, (uintptr_t)base_CEffect};
const uintptr_t base_CGlassWall[] = {(uintptr_t)&type_CGlassWall, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CGlassWall};
const uintptr_t locator_CHealSp[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CHealSp, (uintptr_t)hierarchy_CHealSp};
const uintptr_t hierarchy_CHealSp[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CHealSp};
const uintptr_t bases_CHealSp[] = {(uintptr_t)base_CHealSp, (uintptr_t)base_CEffect};
const uintptr_t base_CHealSp[] = {(uintptr_t)&type_CHealSp, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CHealSp};
const uintptr_t locator_CLevel150Effect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CLevel150Effect, (uintptr_t)hierarchy_CLevel150Effect};
const uintptr_t hierarchy_CLevel150Effect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CLevel150Effect};
const uintptr_t bases_CLevel150Effect[] = {(uintptr_t)base_CLevel150Effect, (uintptr_t)base_CEffect};
const uintptr_t base_CLevel150Effect[] = {(uintptr_t)&type_CLevel150Effect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CLevel150Effect};
const uintptr_t locator_CLevel150SubEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CLevel150SubEffect, (uintptr_t)hierarchy_CLevel150SubEffect};
const uintptr_t hierarchy_CLevel150SubEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CLevel150SubEffect};
const uintptr_t bases_CLevel150SubEffect[] = {(uintptr_t)base_CLevel150SubEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CLevel150SubEffect[] = {(uintptr_t)&type_CLevel150SubEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CLevel150SubEffect};
const uintptr_t locator_CMagicFloorEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CMagicFloorEffect, (uintptr_t)hierarchy_CMagicFloorEffect};
const uintptr_t hierarchy_CMagicFloorEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CMagicFloorEffect};
const uintptr_t bases_CMagicFloorEffect[] = {(uintptr_t)base_CMagicFloorEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CMagicFloorEffect[] = {(uintptr_t)&type_CMagicFloorEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CMagicFloorEffect};
const uintptr_t locator_CMagnumBreak[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CMagnumBreak, (uintptr_t)hierarchy_CMagnumBreak};
const uintptr_t hierarchy_CMagnumBreak[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CMagnumBreak};
const uintptr_t bases_CMagnumBreak[] = {(uintptr_t)base_CMagnumBreak, (uintptr_t)base_CEffect};
const uintptr_t base_CMagnumBreak[] = {(uintptr_t)&type_CMagnumBreak, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CMagnumBreak};
const uintptr_t locator_CMapChainEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CMapChainEffect, (uintptr_t)hierarchy_CMapChainEffect};
const uintptr_t hierarchy_CMapChainEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CMapChainEffect};
const uintptr_t bases_CMapChainEffect[] = {(uintptr_t)base_CMapChainEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CMapChainEffect[] = {(uintptr_t)&type_CMapChainEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CMapChainEffect};
const uintptr_t locator_CMappillarEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CMappillarEffect, (uintptr_t)hierarchy_CMappillarEffect};
const uintptr_t hierarchy_CMappillarEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CMappillarEffect};
const uintptr_t bases_CMappillarEffect[] = {(uintptr_t)base_CMappillarEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CMappillarEffect[] = {(uintptr_t)&type_CMappillarEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CMappillarEffect};
const uintptr_t locator_CSquareRangeEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CSquareRangeEffect, (uintptr_t)hierarchy_CSquareRangeEffect};
const uintptr_t hierarchy_CSquareRangeEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CSquareRangeEffect};
const uintptr_t bases_CSquareRangeEffect[] = {(uintptr_t)base_CSquareRangeEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CSquareRangeEffect[] = {(uintptr_t)&type_CSquareRangeEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CSquareRangeEffect};
const uintptr_t locator_CSteal[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CSteal, (uintptr_t)hierarchy_CSteal};
const uintptr_t hierarchy_CSteal[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CSteal};
const uintptr_t bases_CSteal[] = {(uintptr_t)base_CSteal, (uintptr_t)base_CEffect};
const uintptr_t base_CSteal[] = {(uintptr_t)&type_CSteal, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CSteal};
const uintptr_t locator_CEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CEffect, (uintptr_t)hierarchy_CEffect};
const uintptr_t locator_CElectricEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CElectricEffect, (uintptr_t)hierarchy_CElectricEffect};
const uintptr_t hierarchy_CElectricEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CElectricEffect};
const uintptr_t bases_CElectricEffect[] = {(uintptr_t)base_CElectricEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CElectricEffect[] = {(uintptr_t)&type_CElectricEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CElectricEffect};
const uintptr_t locator_CEmitterEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CEmitterEffect, (uintptr_t)hierarchy_CEmitterEffect};
const uintptr_t locator_CEnergyOrbEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CEnergyOrbEffect, (uintptr_t)hierarchy_CEnergyOrbEffect};
const uintptr_t hierarchy_CEnergyOrbEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CEnergyOrbEffect};
const uintptr_t bases_CEnergyOrbEffect[] = {(uintptr_t)base_CEnergyOrbEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CEnergyOrbEffect[] = {(uintptr_t)&type_CEnergyOrbEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CEnergyOrbEffect};
const uintptr_t locator_CEZ2STREffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CEZ2STREffect, (uintptr_t)hierarchy_CEZ2STREffect};
const uintptr_t hierarchy_CEZ2STREffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CEZ2STREffect};
const uintptr_t bases_CEZ2STREffect[] = {(uintptr_t)base_CEZ2STREffect, (uintptr_t)base_CEffect};
const uintptr_t base_CEZ2STREffect[] = {(uintptr_t)&type_CEZ2STREffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CEZ2STREffect};
const uintptr_t locator_CFlowerCastingEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CFlowerCastingEffect, (uintptr_t)hierarchy_CFlowerCastingEffect};
const uintptr_t hierarchy_CFlowerCastingEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CFlowerCastingEffect};
const uintptr_t bases_CFlowerCastingEffect[] = {(uintptr_t)base_CFlowerCastingEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CFlowerCastingEffect[] = {(uintptr_t)&type_CFlowerCastingEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CFlowerCastingEffect};
const uintptr_t locator_CHealEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CHealEffect, (uintptr_t)hierarchy_CHealEffect};
const uintptr_t hierarchy_CHealEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CHealEffect};
const uintptr_t bases_CHealEffect[] = {(uintptr_t)base_CHealEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CHealEffect[] = {(uintptr_t)&type_CHealEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CHealEffect};
const uintptr_t locator_CLevel99Effect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CLevel99Effect, (uintptr_t)hierarchy_CLevel99Effect};
const uintptr_t hierarchy_CLevel99Effect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CLevel99Effect};
const uintptr_t bases_CLevel99Effect[] = {(uintptr_t)base_CLevel99Effect, (uintptr_t)base_CEffect};
const uintptr_t base_CLevel99Effect[] = {(uintptr_t)&type_CLevel99Effect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CLevel99Effect};
const uintptr_t locator_CLevel99Orb1Effect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CLevel99Orb1Effect, (uintptr_t)hierarchy_CLevel99Orb1Effect};
const uintptr_t hierarchy_CLevel99Orb1Effect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CLevel99Orb1Effect};
const uintptr_t bases_CLevel99Orb1Effect[] = {(uintptr_t)base_CLevel99Orb1Effect, (uintptr_t)base_CEffect};
const uintptr_t base_CLevel99Orb1Effect[] = {(uintptr_t)&type_CLevel99Orb1Effect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CLevel99Orb1Effect};
const uintptr_t locator_CLevel99Orb2Effect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CLevel99Orb2Effect, (uintptr_t)hierarchy_CLevel99Orb2Effect};
const uintptr_t hierarchy_CLevel99Orb2Effect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CLevel99Orb2Effect};
const uintptr_t bases_CLevel99Orb2Effect[] = {(uintptr_t)base_CLevel99Orb2Effect, (uintptr_t)base_CEffect};
const uintptr_t base_CLevel99Orb2Effect[] = {(uintptr_t)&type_CLevel99Orb2Effect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CLevel99Orb2Effect};
const uintptr_t locator_CLightSphereEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CLightSphereEffect, (uintptr_t)hierarchy_CLightSphereEffect};
const uintptr_t hierarchy_CLightSphereEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CLightSphereEffect};
const uintptr_t bases_CLightSphereEffect[] = {(uintptr_t)base_CLightSphereEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CLightSphereEffect[] = {(uintptr_t)&type_CLightSphereEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CLightSphereEffect};
const uintptr_t locator_CMapMagicZoneEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CMapMagicZoneEffect, (uintptr_t)hierarchy_CMapMagicZoneEffect};
const uintptr_t hierarchy_CMapMagicZoneEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CMapMagicZoneEffect};
const uintptr_t bases_CMapMagicZoneEffect[] = {(uintptr_t)base_CMapMagicZoneEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CMapMagicZoneEffect[] = {(uintptr_t)&type_CMapMagicZoneEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CMapMagicZoneEffect};
const uintptr_t locator_CPokJukEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CPokJukEffect, (uintptr_t)hierarchy_CPokJukEffect};
const uintptr_t hierarchy_CPokJukEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CPokJukEffect};
const uintptr_t bases_CPokJukEffect[] = {(uintptr_t)base_CPokJukEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CPokJukEffect[] = {(uintptr_t)&type_CPokJukEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CPokJukEffect};
const uintptr_t locator_CRainbowEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CRainbowEffect, (uintptr_t)hierarchy_CRainbowEffect};
const uintptr_t hierarchy_CRainbowEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CRainbowEffect};
const uintptr_t bases_CRainbowEffect[] = {(uintptr_t)base_CRainbowEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CRainbowEffect[] = {(uintptr_t)&type_CRainbowEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CRainbowEffect};
const uintptr_t locator_CRedLineEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CRedLineEffect, (uintptr_t)hierarchy_CRedLineEffect};
const uintptr_t hierarchy_CRedLineEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CRedLineEffect};
const uintptr_t bases_CRedLineEffect[] = {(uintptr_t)base_CRedLineEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CRedLineEffect[] = {(uintptr_t)&type_CRedLineEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CRedLineEffect};
const uintptr_t locator_CRotateBgEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CRotateBgEffect, (uintptr_t)hierarchy_CRotateBgEffect};
const uintptr_t hierarchy_CRotateBgEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CRotateBgEffect};
const uintptr_t bases_CRotateBgEffect[] = {(uintptr_t)base_CRotateBgEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CRotateBgEffect[] = {(uintptr_t)&type_CRotateBgEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CRotateBgEffect};
const uintptr_t locator_CRotateItemEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CRotateItemEffect, (uintptr_t)hierarchy_CRotateItemEffect};
const uintptr_t hierarchy_CRotateItemEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CRotateItemEffect};
const uintptr_t bases_CRotateItemEffect[] = {(uintptr_t)base_CRotateItemEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CRotateItemEffect[] = {(uintptr_t)&type_CRotateItemEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CRotateItemEffect};
const uintptr_t locator_CRotateLineEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CRotateLineEffect, (uintptr_t)hierarchy_CRotateLineEffect};
const uintptr_t hierarchy_CRotateLineEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CRotateLineEffect};
const uintptr_t bases_CRotateLineEffect[] = {(uintptr_t)base_CRotateLineEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CRotateLineEffect[] = {(uintptr_t)&type_CRotateLineEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CRotateLineEffect};
const uintptr_t locator_CSaintCastingEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CSaintCastingEffect, (uintptr_t)hierarchy_CSaintCastingEffect};
const uintptr_t hierarchy_CSaintCastingEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CSaintCastingEffect};
const uintptr_t bases_CSaintCastingEffect[] = {(uintptr_t)base_CSaintCastingEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CSaintCastingEffect[] = {(uintptr_t)&type_CSaintCastingEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CSaintCastingEffect};
const uintptr_t locator_CScatterEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CScatterEffect, (uintptr_t)hierarchy_CScatterEffect};
const uintptr_t hierarchy_CScatterEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CScatterEffect};
const uintptr_t bases_CScatterEffect[] = {(uintptr_t)base_CScatterEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CScatterEffect[] = {(uintptr_t)&type_CScatterEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CScatterEffect};
const uintptr_t locator_CSmokeEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CSmokeEffect, (uintptr_t)hierarchy_CSmokeEffect};
const uintptr_t hierarchy_CSmokeEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CSmokeEffect};
const uintptr_t bases_CSmokeEffect[] = {(uintptr_t)base_CSmokeEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CSmokeEffect[] = {(uintptr_t)&type_CSmokeEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CSmokeEffect};
const uintptr_t locator_CSphereWind2Effect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CSphereWind2Effect, (uintptr_t)hierarchy_CSphereWind2Effect};
const uintptr_t hierarchy_CSphereWind2Effect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CSphereWind2Effect};
const uintptr_t bases_CSphereWind2Effect[] = {(uintptr_t)base_CSphereWind2Effect, (uintptr_t)base_CEffect};
const uintptr_t base_CSphereWind2Effect[] = {(uintptr_t)&type_CSphereWind2Effect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CSphereWind2Effect};
const uintptr_t locator_CSphereWindEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CSphereWindEffect, (uintptr_t)hierarchy_CSphereWindEffect};
const uintptr_t hierarchy_CSphereWindEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CSphereWindEffect};
const uintptr_t bases_CSphereWindEffect[] = {(uintptr_t)base_CSphereWindEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CSphereWindEffect[] = {(uintptr_t)&type_CSphereWindEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CSphereWindEffect};
const uintptr_t locator_CSpriteEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CSpriteEffect, (uintptr_t)hierarchy_CSpriteEffect};
const uintptr_t hierarchy_CSpriteEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CSpriteEffect};
const uintptr_t bases_CSpriteEffect[] = {(uintptr_t)base_CSpriteEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CSpriteEffect[] = {(uintptr_t)&type_CSpriteEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CSpriteEffect};
const uintptr_t locator_CStarEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CStarEffect, (uintptr_t)hierarchy_CStarEffect};
const uintptr_t hierarchy_CStarEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CStarEffect};
const uintptr_t bases_CStarEffect[] = {(uintptr_t)base_CStarEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CStarEffect[] = {(uintptr_t)&type_CStarEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CStarEffect};
const uintptr_t locator_CStormGustEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CStormGustEffect, (uintptr_t)hierarchy_CStormGustEffect};
const uintptr_t hierarchy_CStormGustEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CStormGustEffect};
const uintptr_t bases_CStormGustEffect[] = {(uintptr_t)base_CStormGustEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CStormGustEffect[] = {(uintptr_t)&type_CStormGustEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CStormGustEffect};
const uintptr_t locator_CStormKickEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CStormKickEffect, (uintptr_t)hierarchy_CStormKickEffect};
const uintptr_t hierarchy_CStormKickEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CStormKickEffect};
const uintptr_t bases_CStormKickEffect[] = {(uintptr_t)base_CStormKickEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CStormKickEffect[] = {(uintptr_t)&type_CStormKickEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CStormKickEffect};
const uintptr_t locator_CThrowItemEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CThrowItemEffect, (uintptr_t)hierarchy_CThrowItemEffect};
const uintptr_t hierarchy_CThrowItemEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CThrowItemEffect};
const uintptr_t bases_CThrowItemEffect[] = {(uintptr_t)base_CThrowItemEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CThrowItemEffect[] = {(uintptr_t)&type_CThrowItemEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CThrowItemEffect};
const uintptr_t locator_CVolcanoEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CVolcanoEffect, (uintptr_t)hierarchy_CVolcanoEffect};
const uintptr_t hierarchy_CVolcanoEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CVolcanoEffect};
const uintptr_t bases_CVolcanoEffect[] = {(uintptr_t)base_CVolcanoEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CVolcanoEffect[] = {(uintptr_t)&type_CVolcanoEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CVolcanoEffect};
const uintptr_t locator_CAstralStrikeEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CAstralStrikeEffect, (uintptr_t)hierarchy_CAstralStrikeEffect};
const uintptr_t hierarchy_CAstralStrikeEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CAstralStrikeEffect};
const uintptr_t bases_CAstralStrikeEffect[] = {(uintptr_t)base_CAstralStrikeEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CAstralStrikeEffect[] = {(uintptr_t)&type_CAstralStrikeEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CAstralStrikeEffect};
const uintptr_t locator_CAMachineEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CAMachineEffect, (uintptr_t)hierarchy_CAMachineEffect};
const uintptr_t hierarchy_CAMachineEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CAMachineEffect};
const uintptr_t bases_CAMachineEffect[] = {(uintptr_t)base_CAMachineEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CAMachineEffect[] = {(uintptr_t)&type_CAMachineEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CAMachineEffect};
const uintptr_t locator_CBlastForge[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CBlastForge, (uintptr_t)hierarchy_CBlastForge};
const uintptr_t hierarchy_CBlastForge[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CBlastForge};
const uintptr_t bases_CBlastForge[] = {(uintptr_t)base_CBlastForge, (uintptr_t)base_CEffect};
const uintptr_t base_CBlastForge[] = {(uintptr_t)&type_CBlastForge, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CBlastForge};
const uintptr_t locator_CCaneOfEvilEyeEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CCaneOfEvilEyeEffect, (uintptr_t)hierarchy_CCaneOfEvilEyeEffect};
const uintptr_t hierarchy_CCaneOfEvilEyeEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CCaneOfEvilEyeEffect};
const uintptr_t bases_CCaneOfEvilEyeEffect[] = {(uintptr_t)base_CCaneOfEvilEyeEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CCaneOfEvilEyeEffect[] = {(uintptr_t)&type_CCaneOfEvilEyeEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CCaneOfEvilEyeEffect};
const uintptr_t locator_CEZ2STREffectEx[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CEZ2STREffectEx, (uintptr_t)hierarchy_CEZ2STREffectEx};
const uintptr_t hierarchy_CEZ2STREffectEx[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CEZ2STREffectEx};
const uintptr_t bases_CEZ2STREffectEx[] = {(uintptr_t)base_CEZ2STREffectEx, (uintptr_t)base_CEffect};
const uintptr_t base_CEZ2STREffectEx[] = {(uintptr_t)&type_CEZ2STREffectEx, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CEZ2STREffectEx};
const uintptr_t locator_CFuumaShouakuEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CFuumaShouakuEffect, (uintptr_t)hierarchy_CFuumaShouakuEffect};
const uintptr_t hierarchy_CFuumaShouakuEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CFuumaShouakuEffect};
const uintptr_t bases_CFuumaShouakuEffect[] = {(uintptr_t)base_CFuumaShouakuEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CFuumaShouakuEffect[] = {(uintptr_t)&type_CFuumaShouakuEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CFuumaShouakuEffect};
const uintptr_t locator_CJupitelThunderStormEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CJupitelThunderStormEffect, (uintptr_t)hierarchy_CJupitelThunderStormEffect};
const uintptr_t hierarchy_CJupitelThunderStormEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CJupitelThunderStormEffect};
const uintptr_t bases_CJupitelThunderStormEffect[] = {(uintptr_t)base_CJupitelThunderStormEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CJupitelThunderStormEffect[] = {(uintptr_t)&type_CJupitelThunderStormEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CJupitelThunderStormEffect};
const uintptr_t locator_CClimaxEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CClimaxEffect, (uintptr_t)hierarchy_CClimaxEffect};
const uintptr_t hierarchy_CClimaxEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CClimaxEffect};
const uintptr_t bases_CClimaxEffect[] = {(uintptr_t)base_CClimaxEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CClimaxEffect[] = {(uintptr_t)&type_CClimaxEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CClimaxEffect};
const uintptr_t locator_CCompetentia[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CCompetentia, (uintptr_t)hierarchy_CCompetentia};
const uintptr_t hierarchy_CCompetentia[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CCompetentia};
const uintptr_t bases_CCompetentia[] = {(uintptr_t)base_CCompetentia, (uintptr_t)base_CEffect};
const uintptr_t base_CCompetentia[] = {(uintptr_t)&type_CCompetentia, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CCompetentia};
const uintptr_t locator_CCresciveBolt[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CCresciveBolt, (uintptr_t)hierarchy_CCresciveBolt};
const uintptr_t hierarchy_CCresciveBolt[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CCresciveBolt};
const uintptr_t bases_CCresciveBolt[] = {(uintptr_t)base_CCresciveBolt, (uintptr_t)base_CEffect};
const uintptr_t base_CCresciveBolt[] = {(uintptr_t)&type_CCresciveBolt, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CCresciveBolt};
const uintptr_t locator_CCrimsonArrowEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CCrimsonArrowEffect, (uintptr_t)hierarchy_CCrimsonArrowEffect};
const uintptr_t hierarchy_CCrimsonArrowEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CCrimsonArrowEffect};
const uintptr_t bases_CCrimsonArrowEffect[] = {(uintptr_t)base_CCrimsonArrowEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CCrimsonArrowEffect[] = {(uintptr_t)&type_CCrimsonArrowEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CCrimsonArrowEffect};
const uintptr_t locator_CCurseOfCubeEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CCurseOfCubeEffect, (uintptr_t)hierarchy_CCurseOfCubeEffect};
const uintptr_t hierarchy_CCurseOfCubeEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CCurseOfCubeEffect};
const uintptr_t bases_CCurseOfCubeEffect[] = {(uintptr_t)base_CCurseOfCubeEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CCurseOfCubeEffect[] = {(uintptr_t)&type_CCurseOfCubeEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CCurseOfCubeEffect};
const uintptr_t locator_CCylinderEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CCylinderEffect, (uintptr_t)hierarchy_CCylinderEffect};
const uintptr_t hierarchy_CCylinderEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CCylinderEffect};
const uintptr_t bases_CCylinderEffect[] = {(uintptr_t)base_CCylinderEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CCylinderEffect[] = {(uintptr_t)&type_CCylinderEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CCylinderEffect};
const uintptr_t locator_CDancingKnifeEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CDancingKnifeEffect, (uintptr_t)hierarchy_CDancingKnifeEffect};
const uintptr_t hierarchy_CDancingKnifeEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CDancingKnifeEffect};
const uintptr_t bases_CDancingKnifeEffect[] = {(uintptr_t)base_CDancingKnifeEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CDancingKnifeEffect[] = {(uintptr_t)&type_CDancingKnifeEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CDancingKnifeEffect};
const uintptr_t locator_CEternalSlashAttackEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CEternalSlashAttackEffect, (uintptr_t)hierarchy_CEternalSlashAttackEffect};
const uintptr_t hierarchy_CEternalSlashAttackEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CEternalSlashAttackEffect};
const uintptr_t bases_CEternalSlashAttackEffect[] = {(uintptr_t)base_CEternalSlashAttackEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CEternalSlashAttackEffect[] = {(uintptr_t)&type_CEternalSlashAttackEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CEternalSlashAttackEffect};
const uintptr_t locator_CEternalSlashCountEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CEternalSlashCountEffect, (uintptr_t)hierarchy_CEternalSlashCountEffect};
const uintptr_t hierarchy_CEternalSlashCountEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CEternalSlashCountEffect};
const uintptr_t bases_CEternalSlashCountEffect[] = {(uintptr_t)base_CEternalSlashCountEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CEternalSlashCountEffect[] = {(uintptr_t)&type_CEternalSlashCountEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CEternalSlashCountEffect};
const uintptr_t locator_CFloralFlareRoad[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CFloralFlareRoad, (uintptr_t)hierarchy_CFloralFlareRoad};
const uintptr_t hierarchy_CFloralFlareRoad[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CFloralFlareRoad};
const uintptr_t bases_CFloralFlareRoad[] = {(uintptr_t)base_CFloralFlareRoad, (uintptr_t)base_CEffect};
const uintptr_t base_CFloralFlareRoad[] = {(uintptr_t)&type_CFloralFlareRoad, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CFloralFlareRoad};
const uintptr_t locator_CFromTheAbyssEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CFromTheAbyssEffect, (uintptr_t)hierarchy_CFromTheAbyssEffect};
const uintptr_t hierarchy_CFromTheAbyssEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CFromTheAbyssEffect};
const uintptr_t bases_CFromTheAbyssEffect[] = {(uintptr_t)base_CFromTheAbyssEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CFromTheAbyssEffect[] = {(uintptr_t)&type_CFromTheAbyssEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CFromTheAbyssEffect};
const uintptr_t locator_CFullScreenEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CFullScreenEffect, (uintptr_t)hierarchy_CFullScreenEffect};
const uintptr_t hierarchy_CFullScreenEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CFullScreenEffect};
const uintptr_t bases_CFullScreenEffect[] = {(uintptr_t)base_CFullScreenEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CFullScreenEffect[] = {(uintptr_t)&type_CFullScreenEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CFullScreenEffect};
const uintptr_t locator_CGaleStorm[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CGaleStorm, (uintptr_t)hierarchy_CGaleStorm};
const uintptr_t hierarchy_CGaleStorm[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CGaleStorm};
const uintptr_t bases_CGaleStorm[] = {(uintptr_t)base_CGaleStorm, (uintptr_t)base_CEffect};
const uintptr_t base_CGaleStorm[] = {(uintptr_t)&type_CGaleStorm, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CGaleStorm};
const uintptr_t locator_CHawkVumerangEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CHawkVumerangEffect, (uintptr_t)hierarchy_CHawkVumerangEffect};
const uintptr_t hierarchy_CHawkVumerangEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CHawkVumerangEffect};
const uintptr_t bases_CHawkVumerangEffect[] = {(uintptr_t)base_CHawkVumerangEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CHawkVumerangEffect[] = {(uintptr_t)&type_CHawkVumerangEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CHawkVumerangEffect};
const uintptr_t locator_CItemDropEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CItemDropEffect, (uintptr_t)hierarchy_CItemDropEffect};
const uintptr_t hierarchy_CItemDropEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CItemDropEffect};
const uintptr_t bases_CItemDropEffect[] = {(uintptr_t)base_CItemDropEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CItemDropEffect[] = {(uintptr_t)&type_CItemDropEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CItemDropEffect};
const uintptr_t locator_CPetitioEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CPetitioEffect, (uintptr_t)hierarchy_CPetitioEffect};
const uintptr_t hierarchy_CPetitioEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CPetitioEffect};
const uintptr_t bases_CPetitioEffect[] = {(uintptr_t)base_CPetitioEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CPetitioEffect[] = {(uintptr_t)&type_CPetitioEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CPetitioEffect};
const uintptr_t locator_CPneumaticusProcellaEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CPneumaticusProcellaEffect, (uintptr_t)hierarchy_CPneumaticusProcellaEffect};
const uintptr_t hierarchy_CPneumaticusProcellaEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CPneumaticusProcellaEffect};
const uintptr_t bases_CPneumaticusProcellaEffect[] = {(uintptr_t)base_CPneumaticusProcellaEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CPneumaticusProcellaEffect[] = {(uintptr_t)&type_CPneumaticusProcellaEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CPneumaticusProcellaEffect};
const uintptr_t locator_CRandomHitEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CRandomHitEffect, (uintptr_t)hierarchy_CRandomHitEffect};
const uintptr_t hierarchy_CRandomHitEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CRandomHitEffect};
const uintptr_t bases_CRandomHitEffect[] = {(uintptr_t)base_CRandomHitEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CRandomHitEffect[] = {(uintptr_t)&type_CRandomHitEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CRandomHitEffect};
const uintptr_t locator_CSavageImpactEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CSavageImpactEffect, (uintptr_t)hierarchy_CSavageImpactEffect};
const uintptr_t hierarchy_CSavageImpactEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CSavageImpactEffect};
const uintptr_t bases_CSavageImpactEffect[] = {(uintptr_t)base_CSavageImpactEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CSavageImpactEffect[] = {(uintptr_t)&type_CSavageImpactEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CSavageImpactEffect};
const uintptr_t locator_CShadowExceed[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CShadowExceed, (uintptr_t)hierarchy_CShadowExceed};
const uintptr_t hierarchy_CShadowExceed[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CShadowExceed};
const uintptr_t bases_CShadowExceed[] = {(uintptr_t)base_CShadowExceed, (uintptr_t)base_CEffect};
const uintptr_t base_CShadowExceed[] = {(uintptr_t)&type_CShadowExceed, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CShadowExceed};
const uintptr_t locator_CStormCannonEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CStormCannonEffect, (uintptr_t)hierarchy_CStormCannonEffect};
const uintptr_t hierarchy_CStormCannonEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CStormCannonEffect};
const uintptr_t bases_CStormCannonEffect[] = {(uintptr_t)base_CStormCannonEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CStormCannonEffect[] = {(uintptr_t)&type_CStormCannonEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CStormCannonEffect};
const uintptr_t locator_CTalismanMarkEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CTalismanMarkEffect, (uintptr_t)hierarchy_CTalismanMarkEffect};
const uintptr_t hierarchy_CTalismanMarkEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CTalismanMarkEffect};
const uintptr_t bases_CTalismanMarkEffect[] = {(uintptr_t)base_CTalismanMarkEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CTalismanMarkEffect[] = {(uintptr_t)&type_CTalismanMarkEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CTalismanMarkEffect};
const uintptr_t locator_CViolentQuakeEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CViolentQuakeEffect, (uintptr_t)hierarchy_CViolentQuakeEffect};
const uintptr_t hierarchy_CViolentQuakeEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CViolentQuakeEffect};
const uintptr_t bases_CViolentQuakeEffect[] = {(uintptr_t)base_CViolentQuakeEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CViolentQuakeEffect[] = {(uintptr_t)&type_CViolentQuakeEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CViolentQuakeEffect};
const uintptr_t locator_CWindEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CWindEffect, (uintptr_t)hierarchy_CWindEffect};
const uintptr_t hierarchy_CWindEffect[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CWindEffect};
const uintptr_t bases_CWindEffect[] = {(uintptr_t)base_CWindEffect, (uintptr_t)base_CEffect};
const uintptr_t base_CWindEffect[] = {(uintptr_t)&type_CWindEffect, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CWindEffect};
const uintptr_t locator_UIDragMiniPartyWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIDragMiniPartyWnd, (uintptr_t)hierarchy_UIDragMiniPartyWnd};
const uintptr_t hierarchy_UIDragMiniPartyWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIDragMiniPartyWnd};
const uintptr_t bases_UIDragMiniPartyWnd[] = {(uintptr_t)base_UIDragMiniPartyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIDragMiniPartyWnd[] = {(uintptr_t)&type_UIDragMiniPartyWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIDragMiniPartyWnd};
const uintptr_t locator_UIFriendOptionWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIFriendOptionWnd, (uintptr_t)hierarchy_UIFriendOptionWnd};
const uintptr_t hierarchy_UIFriendOptionWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIFriendOptionWnd};
const uintptr_t bases_UIFriendOptionWnd[] = {(uintptr_t)base_UIFriendOptionWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIFriendOptionWnd[] = {(uintptr_t)&type_UIFriendOptionWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIFriendOptionWnd};
const uintptr_t locator_UIJobIconButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIJobIconButton, (uintptr_t)hierarchy_UIJobIconButton};
const uintptr_t hierarchy_UIJobIconButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIJobIconButton};
const uintptr_t bases_UIJobIconButton[] = {(uintptr_t)base_UIJobIconButton, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIJobIconButton[] = {(uintptr_t)&type_UIJobIconButton, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIJobIconButton};
const uintptr_t locator_UIMessengerGroupWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMessengerGroupWnd, (uintptr_t)hierarchy_UIMessengerGroupWnd};
const uintptr_t hierarchy_UIMessengerGroupWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMessengerGroupWnd};
const uintptr_t bases_UIMessengerGroupWnd[] = {(uintptr_t)base_UIMessengerGroupWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMessengerGroupWnd[] = {(uintptr_t)&type_UIMessengerGroupWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMessengerGroupWnd};
const uintptr_t locator_UIMiniPartyInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMiniPartyInfoWnd, (uintptr_t)hierarchy_UIMiniPartyInfoWnd};
const uintptr_t hierarchy_UIMiniPartyInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMiniPartyInfoWnd};
const uintptr_t bases_UIMiniPartyInfoWnd[] = {(uintptr_t)base_UIMiniPartyInfoWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMiniPartyInfoWnd[] = {(uintptr_t)&type_UIMiniPartyInfoWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMiniPartyInfoWnd};
const uintptr_t locator_UIMiniPartyWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMiniPartyWnd, (uintptr_t)hierarchy_UIMiniPartyWnd};
const uintptr_t hierarchy_UIMiniPartyWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMiniPartyWnd};
const uintptr_t bases_UIMiniPartyWnd[] = {(uintptr_t)base_UIMiniPartyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMiniPartyWnd[] = {(uintptr_t)&type_UIMiniPartyWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMiniPartyWnd};
const uintptr_t locator_CActRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CActRes, (uintptr_t)hierarchy_CActRes};
const uintptr_t hierarchy_CActRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CActRes};
const uintptr_t bases_CActRes[] = {(uintptr_t)base_CActRes, (uintptr_t)base_CRes};
const uintptr_t base_CActRes[] = {(uintptr_t)&type_CActRes, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CActRes};
const uintptr_t locator_C3dAttr[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_C3dAttr, (uintptr_t)hierarchy_C3dAttr};
const uintptr_t hierarchy_C3dAttr[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_C3dAttr};
const uintptr_t bases_C3dAttr[] = {(uintptr_t)base_C3dAttr, (uintptr_t)base_CRes};
const uintptr_t base_C3dAttr[] = {(uintptr_t)&type_C3dAttr, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_C3dAttr};
const uintptr_t locator_CBitmapRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CBitmapRes, (uintptr_t)hierarchy_CBitmapRes};
const uintptr_t hierarchy_CBitmapRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CBitmapRes};
const uintptr_t bases_CBitmapRes[] = {(uintptr_t)base_CBitmapRes, (uintptr_t)base_CRes};
const uintptr_t base_CBitmapRes[] = {(uintptr_t)&type_CBitmapRes, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CBitmapRes};
const uintptr_t locator_CGifRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CGifRes, (uintptr_t)hierarchy_CGifRes};
const uintptr_t hierarchy_CGifRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_CGifRes};
const uintptr_t bases_CGifRes[] = {(uintptr_t)base_CGifRes, (uintptr_t)base_CBitmapRes, (uintptr_t)base_CRes};
const uintptr_t base_CGifRes[] = {(uintptr_t)&type_CGifRes, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CGifRes};
const uintptr_t locator_CEmblemRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CEmblemRes, (uintptr_t)hierarchy_CEmblemRes};
const uintptr_t hierarchy_CEmblemRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CEmblemRes};
const uintptr_t bases_CEmblemRes[] = {(uintptr_t)base_CEmblemRes, (uintptr_t)base_CRes};
const uintptr_t base_CEmblemRes[] = {(uintptr_t)&type_CEmblemRes, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CEmblemRes};
const uintptr_t locator_CEZeffectRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CEZeffectRes, (uintptr_t)hierarchy_CEZeffectRes};
const uintptr_t hierarchy_CEZeffectRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CEZeffectRes};
const uintptr_t bases_CEZeffectRes[] = {(uintptr_t)base_CEZeffectRes, (uintptr_t)base_CRes};
const uintptr_t base_CEZeffectRes[] = {(uintptr_t)&type_CEZeffectRes, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CEZeffectRes};
const uintptr_t locator_CGndRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CGndRes, (uintptr_t)hierarchy_CGndRes};
const uintptr_t hierarchy_CGndRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CGndRes};
const uintptr_t bases_CGndRes[] = {(uintptr_t)base_CGndRes, (uintptr_t)base_CRes};
const uintptr_t base_CGndRes[] = {(uintptr_t)&type_CGndRes, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CGndRes};
const uintptr_t locator_CImfRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CImfRes, (uintptr_t)hierarchy_CImfRes};
const uintptr_t hierarchy_CImfRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CImfRes};
const uintptr_t bases_CImfRes[] = {(uintptr_t)base_CImfRes, (uintptr_t)base_CRes};
const uintptr_t base_CImfRes[] = {(uintptr_t)&type_CImfRes, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CImfRes};
const uintptr_t hierarchy_C3dModelRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_C3dModelRes};
const uintptr_t bases_C3dModelRes[] = {(uintptr_t)base_C3dModelRes, (uintptr_t)base_CRes};
const uintptr_t base_C3dModelRes[] = {(uintptr_t)&type_C3dModelRes, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_C3dModelRes};
const uintptr_t locator_C3dModelResV1[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_C3dModelResV1, (uintptr_t)hierarchy_C3dModelResV1};
const uintptr_t hierarchy_C3dModelResV1[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_C3dModelResV1};
const uintptr_t bases_C3dModelResV1[] = {(uintptr_t)base_C3dModelResV1, (uintptr_t)base_C3dModelRes, (uintptr_t)base_CRes};
const uintptr_t base_C3dModelResV1[] = {(uintptr_t)&type_C3dModelResV1, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_C3dModelResV1};
const uintptr_t locator_C3dModelResV2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_C3dModelResV2, (uintptr_t)hierarchy_C3dModelResV2};
const uintptr_t hierarchy_C3dModelResV2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_C3dModelResV2};
const uintptr_t bases_C3dModelResV2[] = {(uintptr_t)base_C3dModelResV2, (uintptr_t)base_C3dModelRes, (uintptr_t)base_CRes};
const uintptr_t base_C3dModelResV2[] = {(uintptr_t)&type_C3dModelResV2, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_C3dModelResV2};
const uintptr_t locator_C3dAnimModelRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_C3dAnimModelRes, (uintptr_t)hierarchy_C3dAnimModelRes};
const uintptr_t hierarchy_C3dAnimModelRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_C3dAnimModelRes};
const uintptr_t bases_C3dAnimModelRes[] = {(uintptr_t)base_C3dAnimModelRes, (uintptr_t)base_CRes};
const uintptr_t base_C3dAnimModelRes[] = {(uintptr_t)&type_C3dAnimModelRes, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_C3dAnimModelRes};
const uintptr_t locator_C3dGrannyModelRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_C3dGrannyModelRes, (uintptr_t)hierarchy_C3dGrannyModelRes};
const uintptr_t hierarchy_C3dGrannyModelRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_C3dGrannyModelRes};
const uintptr_t bases_C3dGrannyModelRes[] = {(uintptr_t)base_C3dGrannyModelRes, (uintptr_t)base_CRes};
const uintptr_t base_C3dGrannyModelRes[] = {(uintptr_t)&type_C3dGrannyModelRes, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_C3dGrannyModelRes};
const uintptr_t locator_C3dModelRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_C3dModelRes, (uintptr_t)hierarchy_C3dModelRes};
const uintptr_t locator_CPaletteRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CPaletteRes, (uintptr_t)hierarchy_CPaletteRes};
const uintptr_t hierarchy_CPaletteRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CPaletteRes};
const uintptr_t bases_CPaletteRes[] = {(uintptr_t)base_CPaletteRes, (uintptr_t)base_CRes};
const uintptr_t base_CPaletteRes[] = {(uintptr_t)&type_CPaletteRes, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CPaletteRes};
const uintptr_t locator_CSprRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CSprRes, (uintptr_t)hierarchy_CSprRes};
const uintptr_t hierarchy_CSprRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CSprRes};
const uintptr_t bases_CSprRes[] = {(uintptr_t)base_CSprRes, (uintptr_t)base_CRes};
const uintptr_t base_CSprRes[] = {(uintptr_t)&type_CSprRes, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CSprRes};
const uintptr_t locator_CWave[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CWave, (uintptr_t)hierarchy_CWave};
const uintptr_t hierarchy_CWave[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_CWave};
const uintptr_t bases_CWave[] = {(uintptr_t)base_CWave, (uintptr_t)base_CRes};
const uintptr_t base_CWave[] = {(uintptr_t)&type_CWave, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CWave};
const uintptr_t locator_C3dWorldRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_C3dWorldRes, (uintptr_t)hierarchy_C3dWorldRes};
const uintptr_t hierarchy_C3dWorldRes[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)2, (uintptr_t)bases_C3dWorldRes};
const uintptr_t bases_C3dWorldRes[] = {(uintptr_t)base_C3dWorldRes, (uintptr_t)base_CRes};
const uintptr_t base_C3dWorldRes[] = {(uintptr_t)&type_C3dWorldRes, (uintptr_t)1, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_C3dWorldRes};
const uintptr_t locator_UIAchScrollBar[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIAchScrollBar, (uintptr_t)hierarchy_UIAchScrollBar};
const uintptr_t hierarchy_UIAchScrollBar[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIAchScrollBar};
const uintptr_t bases_UIAchScrollBar[] = {(uintptr_t)base_UIAchScrollBar, (uintptr_t)base_UIScrollBar, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIAchScrollBar[] = {(uintptr_t)&type_UIAchScrollBar, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIAchScrollBar};
const uintptr_t locator_UIAchTV[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIAchTV, (uintptr_t)hierarchy_UIAchTV};
const uintptr_t hierarchy_UIAchTV[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIAchTV};
const uintptr_t bases_UIAchTV[] = {(uintptr_t)base_UIAchTV, (uintptr_t)base_UITextViewer, (uintptr_t)base_UIListBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIAchTV[] = {(uintptr_t)&type_UIAchTV, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIAchTV};
const uintptr_t locator_UIAchTab[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIAchTab, (uintptr_t)hierarchy_UIAchTab};
const uintptr_t hierarchy_UIAchTab[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIAchTab};
const uintptr_t bases_UIAchTab[] = {(uintptr_t)base_UIAchTab, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIAchTab[] = {(uintptr_t)&type_UIAchTab, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIAchTab};
const uintptr_t locator_UICustomGageBar[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UICustomGageBar, (uintptr_t)hierarchy_UICustomGageBar};
const uintptr_t hierarchy_UICustomGageBar[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UICustomGageBar};
const uintptr_t bases_UICustomGageBar[] = {(uintptr_t)base_UICustomGageBar, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UICustomGageBar[] = {(uintptr_t)&type_UICustomGageBar, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UICustomGageBar};
const uintptr_t locator_UIAchUpperWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIAchUpperWnd, (uintptr_t)hierarchy_UIAchUpperWnd};
const uintptr_t hierarchy_UIAchUpperWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIAchUpperWnd};
const uintptr_t bases_UIAchUpperWnd[] = {(uintptr_t)base_UIAchUpperWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIAchUpperWnd[] = {(uintptr_t)&type_UIAchUpperWnd, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIAchUpperWnd};
const uintptr_t locator_UIToolTipBitmapWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIToolTipBitmapWnd, (uintptr_t)hierarchy_UIToolTipBitmapWnd};
const uintptr_t hierarchy_UIToolTipBitmapWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIToolTipBitmapWnd};
const uintptr_t bases_UIToolTipBitmapWnd[] = {(uintptr_t)base_UIToolTipBitmapWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIToolTipBitmapWnd[] = {(uintptr_t)&type_UIToolTipBitmapWnd, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIToolTipBitmapWnd};
const uintptr_t locator_UIAchViewWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIAchViewWnd, (uintptr_t)hierarchy_UIAchViewWnd};
const uintptr_t hierarchy_UIAchViewWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIAchViewWnd};
const uintptr_t bases_UIAchViewWnd[] = {(uintptr_t)base_UIAchViewWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIAchViewWnd[] = {(uintptr_t)&type_UIAchViewWnd, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIAchViewWnd};
const uintptr_t locator_UIAchListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIAchListWnd, (uintptr_t)hierarchy_UIAchListWnd};
const uintptr_t hierarchy_UIAchListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIAchListWnd};
const uintptr_t bases_UIAchListWnd[] = {(uintptr_t)base_UIAchListWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIAchListWnd[] = {(uintptr_t)&type_UIAchListWnd, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIAchListWnd};
const uintptr_t locator_UIAchSummaryWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIAchSummaryWnd, (uintptr_t)hierarchy_UIAchSummaryWnd};
const uintptr_t hierarchy_UIAchSummaryWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIAchSummaryWnd};
const uintptr_t bases_UIAchSummaryWnd[] = {(uintptr_t)base_UIAchSummaryWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIAchSummaryWnd[] = {(uintptr_t)&type_UIAchSummaryWnd, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIAchSummaryWnd};
const uintptr_t locator_UIAchievementWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIAchievementWnd, (uintptr_t)hierarchy_UIAchievementWnd};
const uintptr_t hierarchy_UIAchievementWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIAchievementWnd};
const uintptr_t bases_UIAchievementWnd[] = {(uintptr_t)base_UIAchievementWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIAchievementWnd[] = {(uintptr_t)&type_UIAchievementWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIAchievementWnd};
const uintptr_t locator_UIAchTracingWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIAchTracingWnd, (uintptr_t)hierarchy_UIAchTracingWnd};
const uintptr_t hierarchy_UIAchTracingWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIAchTracingWnd};
const uintptr_t bases_UIAchTracingWnd[] = {(uintptr_t)base_UIAchTracingWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIAchTracingWnd[] = {(uintptr_t)&type_UIAchTracingWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIAchTracingWnd};
const uintptr_t locator_UIJobSlotButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIJobSlotButton, (uintptr_t)hierarchy_UIJobSlotButton};
const uintptr_t hierarchy_UIJobSlotButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIJobSlotButton};
const uintptr_t bases_UIJobSlotButton[] = {(uintptr_t)base_UIJobSlotButton, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIJobSlotButton[] = {(uintptr_t)&type_UIJobSlotButton, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIJobSlotButton};
const uintptr_t locator_UIRegisterPartyWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRegisterPartyWnd, (uintptr_t)hierarchy_UIRegisterPartyWnd};
const uintptr_t hierarchy_UIRegisterPartyWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIRegisterPartyWnd};
const uintptr_t bases_UIRegisterPartyWnd[] = {(uintptr_t)base_UIRegisterPartyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRegisterPartyWnd[] = {(uintptr_t)&type_UIRegisterPartyWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRegisterPartyWnd};
const uintptr_t locator_UIApplyForPartyWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIApplyForPartyWnd, (uintptr_t)hierarchy_UIApplyForPartyWnd};
const uintptr_t hierarchy_UIApplyForPartyWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIApplyForPartyWnd};
const uintptr_t bases_UIApplyForPartyWnd[] = {(uintptr_t)base_UIApplyForPartyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIApplyForPartyWnd[] = {(uintptr_t)&type_UIApplyForPartyWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIApplyForPartyWnd};
const uintptr_t locator_UIPartyInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIPartyInfoWnd, (uintptr_t)hierarchy_UIPartyInfoWnd};
const uintptr_t hierarchy_UIPartyInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIPartyInfoWnd};
const uintptr_t bases_UIPartyInfoWnd[] = {(uintptr_t)base_UIPartyInfoWnd, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIPartyInfoWnd[] = {(uintptr_t)&type_UIPartyInfoWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIPartyInfoWnd};
const uintptr_t locator_UIAdvenPartyBoardWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIAdvenPartyBoardWnd, (uintptr_t)hierarchy_UIAdvenPartyBoardWnd};
const uintptr_t hierarchy_UIAdvenPartyBoardWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIAdvenPartyBoardWnd};
const uintptr_t bases_UIAdvenPartyBoardWnd[] = {(uintptr_t)base_UIAdvenPartyBoardWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIAdvenPartyBoardWnd[] = {(uintptr_t)&type_UIAdvenPartyBoardWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIAdvenPartyBoardWnd};
const uintptr_t locator_UIRequestJoinPartyWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRequestJoinPartyWnd, (uintptr_t)hierarchy_UIRequestJoinPartyWnd};
const uintptr_t hierarchy_UIRequestJoinPartyWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIRequestJoinPartyWnd};
const uintptr_t bases_UIRequestJoinPartyWnd[] = {(uintptr_t)base_UIRequestJoinPartyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRequestJoinPartyWnd[] = {(uintptr_t)&type_UIRequestJoinPartyWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRequestJoinPartyWnd};
const uintptr_t locator_CMergeItemWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CMergeItemWnd, (uintptr_t)hierarchy_CMergeItemWnd};
const uintptr_t hierarchy_CMergeItemWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_CMergeItemWnd};
const uintptr_t bases_CMergeItemWnd[] = {(uintptr_t)base_CMergeItemWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_CMergeItemWnd[] = {(uintptr_t)&type_CMergeItemWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CMergeItemWnd};
const uintptr_t locator_UIBartermarketWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBartermarketWnd, (uintptr_t)hierarchy_UIBartermarketWnd};
const uintptr_t hierarchy_UIBartermarketWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_UIBartermarketWnd};
const uintptr_t bases_UIBartermarketWnd[] = {(uintptr_t)base_UIBartermarketWnd, (uintptr_t)base_UIItemShopWnd, (uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBartermarketWnd[] = {(uintptr_t)&type_UIBartermarketWnd, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBartermarketWnd};
const uintptr_t locator_UIBarterItemPurchaseWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBarterItemPurchaseWnd, (uintptr_t)hierarchy_UIBarterItemPurchaseWnd};
const uintptr_t hierarchy_UIBarterItemPurchaseWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_UIBarterItemPurchaseWnd};
const uintptr_t bases_UIBarterItemPurchaseWnd[] = {(uintptr_t)base_UIBarterItemPurchaseWnd, (uintptr_t)base_UIItemPurchaseWnd, (uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBarterItemPurchaseWnd[] = {(uintptr_t)&type_UIBarterItemPurchaseWnd, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBarterItemPurchaseWnd};
const uintptr_t locator_UIPurchaseButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIPurchaseButton, (uintptr_t)hierarchy_UIPurchaseButton};
const uintptr_t hierarchy_UIPurchaseButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIPurchaseButton};
const uintptr_t bases_UIPurchaseButton[] = {(uintptr_t)base_UIPurchaseButton, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIPurchaseButton[] = {(uintptr_t)&type_UIPurchaseButton, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIPurchaseButton};
const uintptr_t locator_UIExpandedBartermarketWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIExpandedBartermarketWnd, (uintptr_t)hierarchy_UIExpandedBartermarketWnd};
const uintptr_t hierarchy_UIExpandedBartermarketWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIExpandedBartermarketWnd};
const uintptr_t bases_UIExpandedBartermarketWnd[] = {(uintptr_t)base_UIExpandedBartermarketWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIExpandedBartermarketWnd[] = {(uintptr_t)&type_UIExpandedBartermarketWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIExpandedBartermarketWnd};
const uintptr_t locator_UIExpandedBarterItemPurchaseWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIExpandedBarterItemPurchaseWnd, (uintptr_t)hierarchy_UIExpandedBarterItemPurchaseWnd};
const uintptr_t hierarchy_UIExpandedBarterItemPurchaseWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIExpandedBarterItemPurchaseWnd};
const uintptr_t bases_UIExpandedBarterItemPurchaseWnd[] = {(uintptr_t)base_UIExpandedBarterItemPurchaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIExpandedBarterItemPurchaseWnd[] = {(uintptr_t)&type_UIExpandedBarterItemPurchaseWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIExpandedBarterItemPurchaseWnd};
const uintptr_t locator_UIExpandedBarterPurchaseButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIExpandedBarterPurchaseButton, (uintptr_t)hierarchy_UIExpandedBarterPurchaseButton};
const uintptr_t hierarchy_UIExpandedBarterPurchaseButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIExpandedBarterPurchaseButton};
const uintptr_t bases_UIExpandedBarterPurchaseButton[] = {(uintptr_t)base_UIExpandedBarterPurchaseButton, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIExpandedBarterPurchaseButton[] = {(uintptr_t)&type_UIExpandedBarterPurchaseButton, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIExpandedBarterPurchaseButton};
const uintptr_t locator_UIMenuIcon[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMenuIcon, (uintptr_t)hierarchy_UIMenuIcon};
const uintptr_t hierarchy_UIMenuIcon[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIMenuIcon};
const uintptr_t bases_UIMenuIcon[] = {(uintptr_t)base_UIMenuIcon, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMenuIcon[] = {(uintptr_t)&type_UIMenuIcon, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMenuIcon};
const uintptr_t locator_UIMenuIconWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMenuIconWnd, (uintptr_t)hierarchy_UIMenuIconWnd};
const uintptr_t hierarchy_UIMenuIconWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMenuIconWnd};
const uintptr_t bases_UIMenuIconWnd[] = {(uintptr_t)base_UIMenuIconWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMenuIconWnd[] = {(uintptr_t)&type_UIMenuIconWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMenuIconWnd};
const uintptr_t locator_UIClanInfoManageWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIClanInfoManageWnd, (uintptr_t)hierarchy_UIClanInfoManageWnd};
const uintptr_t hierarchy_UIClanInfoManageWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIClanInfoManageWnd};
const uintptr_t bases_UIClanInfoManageWnd[] = {(uintptr_t)base_UIClanInfoManageWnd, (uintptr_t)base_UIClanWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIClanInfoManageWnd[] = {(uintptr_t)&type_UIClanInfoManageWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIClanInfoManageWnd};
const uintptr_t base_UIClanWnd[] = {(uintptr_t)&type_UIClanWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIClanWnd};
const uintptr_t hierarchy_UIClanWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIClanWnd};
const uintptr_t bases_UIClanWnd[] = {(uintptr_t)base_UIClanWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t locator_UIClanWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIClanWnd, (uintptr_t)hierarchy_UIClanWnd};
const uintptr_t locator_UIEmotionListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEmotionListWnd, (uintptr_t)hierarchy_UIEmotionListWnd};
const uintptr_t hierarchy_UIEmotionListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIEmotionListWnd};
const uintptr_t bases_UIEmotionListWnd[] = {(uintptr_t)base_UIEmotionListWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEmotionListWnd[] = {(uintptr_t)&type_UIEmotionListWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEmotionListWnd};
const uintptr_t locator_UIBarGraph4[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBarGraph4, (uintptr_t)hierarchy_UIBarGraph4};
const uintptr_t hierarchy_UIBarGraph4[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIBarGraph4};
const uintptr_t bases_UIBarGraph4[] = {(uintptr_t)base_UIBarGraph4, (uintptr_t)base_UIBarGraph3, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBarGraph4[] = {(uintptr_t)&type_UIBarGraph4, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBarGraph4};
const uintptr_t locator_UILoginWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UILoginWnd, (uintptr_t)hierarchy_UILoginWnd};
const uintptr_t hierarchy_UILoginWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UILoginWnd};
const uintptr_t bases_UILoginWnd[] = {(uintptr_t)base_UILoginWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UILoginWnd[] = {(uintptr_t)&type_UILoginWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UILoginWnd};
const uintptr_t locator_UISelectServerWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISelectServerWnd, (uintptr_t)hierarchy_UISelectServerWnd};
const uintptr_t hierarchy_UISelectServerWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UISelectServerWnd};
const uintptr_t bases_UISelectServerWnd[] = {(uintptr_t)base_UISelectServerWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISelectServerWnd[] = {(uintptr_t)&type_UISelectServerWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISelectServerWnd};
const uintptr_t locator_UIMakeCharWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMakeCharWnd, (uintptr_t)hierarchy_UIMakeCharWnd};
const uintptr_t hierarchy_UIMakeCharWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMakeCharWnd};
const uintptr_t bases_UIMakeCharWnd[] = {(uintptr_t)base_UIMakeCharWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMakeCharWnd[] = {(uintptr_t)&type_UIMakeCharWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMakeCharWnd};
const uintptr_t locator_UIFrameWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIFrameWnd, (uintptr_t)hierarchy_UIFrameWnd};
const uintptr_t locator_UIMessageBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMessageBox, (uintptr_t)hierarchy_UIMessageBox};
const uintptr_t hierarchy_UIMessageBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMessageBox};
const uintptr_t bases_UIMessageBox[] = {(uintptr_t)base_UIMessageBox, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMessageBox[] = {(uintptr_t)&type_UIMessageBox, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMessageBox};
const uintptr_t locator_UIMessageBoxAutoreturn[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMessageBoxAutoreturn, (uintptr_t)hierarchy_UIMessageBoxAutoreturn};
const uintptr_t hierarchy_UIMessageBoxAutoreturn[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMessageBoxAutoreturn};
const uintptr_t bases_UIMessageBoxAutoreturn[] = {(uintptr_t)base_UIMessageBoxAutoreturn, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMessageBoxAutoreturn[] = {(uintptr_t)&type_UIMessageBoxAutoreturn, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMessageBoxAutoreturn};
const uintptr_t locator_UILoginOTPWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UILoginOTPWnd, (uintptr_t)hierarchy_UILoginOTPWnd};
const uintptr_t hierarchy_UILoginOTPWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UILoginOTPWnd};
const uintptr_t bases_UILoginOTPWnd[] = {(uintptr_t)base_UILoginOTPWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UILoginOTPWnd[] = {(uintptr_t)&type_UILoginOTPWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UILoginOTPWnd};
const uintptr_t locator_UINoticeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINoticeWnd, (uintptr_t)hierarchy_UINoticeWnd};
const uintptr_t hierarchy_UINoticeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UINoticeWnd};
const uintptr_t bases_UINoticeWnd[] = {(uintptr_t)base_UINoticeWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINoticeWnd[] = {(uintptr_t)&type_UINoticeWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINoticeWnd};
const uintptr_t locator_UICntWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UICntWnd, (uintptr_t)hierarchy_UICntWnd};
const uintptr_t hierarchy_UICntWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UICntWnd};
const uintptr_t bases_UICntWnd[] = {(uintptr_t)base_UICntWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UICntWnd[] = {(uintptr_t)&type_UICntWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UICntWnd};
const uintptr_t locator_UIAllyGuildAcceptWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIAllyGuildAcceptWnd, (uintptr_t)hierarchy_UIAllyGuildAcceptWnd};
const uintptr_t hierarchy_UIAllyGuildAcceptWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_UIAllyGuildAcceptWnd};
const uintptr_t bases_UIAllyGuildAcceptWnd[] = {(uintptr_t)base_UIAllyGuildAcceptWnd, (uintptr_t)base_UIExchangeAcceptWnd, (uintptr_t)base_UIChooseSellBuyWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIAllyGuildAcceptWnd[] = {(uintptr_t)&type_UIAllyGuildAcceptWnd, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIAllyGuildAcceptWnd};
const uintptr_t locator_UIChatRoomWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIChatRoomWnd, (uintptr_t)hierarchy_UIChatRoomWnd};
const uintptr_t hierarchy_UIChatRoomWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIChatRoomWnd};
const uintptr_t bases_UIChatRoomWnd[] = {(uintptr_t)base_UIChatRoomWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIChatRoomWnd[] = {(uintptr_t)&type_UIChatRoomWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIChatRoomWnd};
const uintptr_t locator_UIMonsterInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMonsterInfoWnd, (uintptr_t)hierarchy_UIMonsterInfoWnd};
const uintptr_t hierarchy_UIMonsterInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMonsterInfoWnd};
const uintptr_t bases_UIMonsterInfoWnd[] = {(uintptr_t)base_UIMonsterInfoWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMonsterInfoWnd[] = {(uintptr_t)&type_UIMonsterInfoWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMonsterInfoWnd};
const uintptr_t locator_UITalkboxTrapInputWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UITalkboxTrapInputWnd, (uintptr_t)hierarchy_UITalkboxTrapInputWnd};
const uintptr_t hierarchy_UITalkboxTrapInputWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UITalkboxTrapInputWnd};
const uintptr_t bases_UITalkboxTrapInputWnd[] = {(uintptr_t)base_UITalkboxTrapInputWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UITalkboxTrapInputWnd[] = {(uintptr_t)&type_UITalkboxTrapInputWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UITalkboxTrapInputWnd};
const uintptr_t locator_UIPetInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIPetInfoWnd, (uintptr_t)hierarchy_UIPetInfoWnd};
const uintptr_t hierarchy_UIPetInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIPetInfoWnd};
const uintptr_t bases_UIPetInfoWnd[] = {(uintptr_t)base_UIPetInfoWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIPetInfoWnd[] = {(uintptr_t)&type_UIPetInfoWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIPetInfoWnd};
const uintptr_t locator_UIHomunInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIHomunInfoWnd, (uintptr_t)hierarchy_UIHomunInfoWnd};
const uintptr_t hierarchy_UIHomunInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIHomunInfoWnd};
const uintptr_t bases_UIHomunInfoWnd[] = {(uintptr_t)base_UIHomunInfoWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIHomunInfoWnd[] = {(uintptr_t)&type_UIHomunInfoWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIHomunInfoWnd};
const uintptr_t locator_UIMerInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMerInfoWnd, (uintptr_t)hierarchy_UIMerInfoWnd};
const uintptr_t hierarchy_UIMerInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMerInfoWnd};
const uintptr_t bases_UIMerInfoWnd[] = {(uintptr_t)base_UIMerInfoWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMerInfoWnd[] = {(uintptr_t)&type_UIMerInfoWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMerInfoWnd};
const uintptr_t locator_UIReplayButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIReplayButton, (uintptr_t)hierarchy_UIReplayButton};
const uintptr_t hierarchy_UIReplayButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIReplayButton};
const uintptr_t bases_UIReplayButton[] = {(uintptr_t)base_UIReplayButton, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIReplayButton[] = {(uintptr_t)&type_UIReplayButton, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIReplayButton};
const uintptr_t locator_UIReplayControlWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIReplayControlWnd, (uintptr_t)hierarchy_UIReplayControlWnd};
const uintptr_t hierarchy_UIReplayControlWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIReplayControlWnd};
const uintptr_t bases_UIReplayControlWnd[] = {(uintptr_t)base_UIReplayControlWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIReplayControlWnd[] = {(uintptr_t)&type_UIReplayControlWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIReplayControlWnd};
const uintptr_t locator_UIReplayListBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIReplayListBox, (uintptr_t)hierarchy_UIReplayListBox};
const uintptr_t hierarchy_UIReplayListBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIReplayListBox};
const uintptr_t bases_UIReplayListBox[] = {(uintptr_t)base_UIReplayListBox, (uintptr_t)base_UIListBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIReplayListBox[] = {(uintptr_t)&type_UIReplayListBox, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIReplayListBox};
const uintptr_t locator_UISelectReplayDataWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISelectReplayDataWnd, (uintptr_t)hierarchy_UISelectReplayDataWnd};
const uintptr_t hierarchy_UISelectReplayDataWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UISelectReplayDataWnd};
const uintptr_t bases_UISelectReplayDataWnd[] = {(uintptr_t)base_UISelectReplayDataWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISelectReplayDataWnd[] = {(uintptr_t)&type_UISelectReplayDataWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISelectReplayDataWnd};
const uintptr_t locator_UIReplayRECControlWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIReplayRECControlWnd, (uintptr_t)hierarchy_UIReplayRECControlWnd};
const uintptr_t hierarchy_UIReplayRECControlWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIReplayRECControlWnd};
const uintptr_t bases_UIReplayRECControlWnd[] = {(uintptr_t)base_UIReplayRECControlWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIReplayRECControlWnd[] = {(uintptr_t)&type_UIReplayRECControlWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIReplayRECControlWnd};
const uintptr_t locator_UIBank_NewWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBank_NewWnd, (uintptr_t)hierarchy_UIBank_NewWnd};
const uintptr_t hierarchy_UIBank_NewWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIBank_NewWnd};
const uintptr_t bases_UIBank_NewWnd[] = {(uintptr_t)base_UIBank_NewWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBank_NewWnd[] = {(uintptr_t)&type_UIBank_NewWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBank_NewWnd};
const uintptr_t locator_UINormalMB[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINormalMB, (uintptr_t)hierarchy_UINormalMB};
const uintptr_t hierarchy_UINormalMB[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UINormalMB};
const uintptr_t bases_UINormalMB[] = {(uintptr_t)base_UINormalMB, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINormalMB[] = {(uintptr_t)&type_UINormalMB, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINormalMB};
const uintptr_t locator_UIQuestDisplay[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIQuestDisplay, (uintptr_t)hierarchy_UIQuestDisplay};
const uintptr_t hierarchy_UIQuestDisplay[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIQuestDisplay};
const uintptr_t bases_UIQuestDisplay[] = {(uintptr_t)base_UIQuestDisplay, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIQuestDisplay[] = {(uintptr_t)&type_UIQuestDisplay, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIQuestDisplay};
const uintptr_t locator_UIGuild_Storage_Log[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIGuild_Storage_Log, (uintptr_t)hierarchy_UIGuild_Storage_Log};
const uintptr_t hierarchy_UIGuild_Storage_Log[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIGuild_Storage_Log};
const uintptr_t bases_UIGuild_Storage_Log[] = {(uintptr_t)base_UIGuild_Storage_Log, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIGuild_Storage_Log[] = {(uintptr_t)&type_UIGuild_Storage_Log, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIGuild_Storage_Log};
const uintptr_t locator_UIOpenMailBoxWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIOpenMailBoxWnd, (uintptr_t)hierarchy_UIOpenMailBoxWnd};
const uintptr_t hierarchy_UIOpenMailBoxWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIOpenMailBoxWnd};
const uintptr_t bases_UIOpenMailBoxWnd[] = {(uintptr_t)base_UIOpenMailBoxWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIOpenMailBoxWnd[] = {(uintptr_t)&type_UIOpenMailBoxWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIOpenMailBoxWnd};
const uintptr_t locator_UISecondCostumeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISecondCostumeWnd, (uintptr_t)hierarchy_UISecondCostumeWnd};
const uintptr_t hierarchy_UISecondCostumeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UISecondCostumeWnd};
const uintptr_t bases_UISecondCostumeWnd[] = {(uintptr_t)base_UISecondCostumeWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISecondCostumeWnd[] = {(uintptr_t)&type_UISecondCostumeWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISecondCostumeWnd};
const uintptr_t locator_UIMileageWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMileageWnd, (uintptr_t)hierarchy_UIMileageWnd};
const uintptr_t hierarchy_UIMileageWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMileageWnd};
const uintptr_t bases_UIMileageWnd[] = {(uintptr_t)base_UIMileageWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMileageWnd[] = {(uintptr_t)&type_UIMileageWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMileageWnd};
const uintptr_t locator_UIOpenRoulletteWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIOpenRoulletteWnd, (uintptr_t)hierarchy_UIOpenRoulletteWnd};
const uintptr_t hierarchy_UIOpenRoulletteWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIOpenRoulletteWnd};
const uintptr_t bases_UIOpenRoulletteWnd[] = {(uintptr_t)base_UIOpenRoulletteWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIOpenRoulletteWnd[] = {(uintptr_t)&type_UIOpenRoulletteWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIOpenRoulletteWnd};
const uintptr_t locator_UITransMenuWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UITransMenuWnd, (uintptr_t)hierarchy_UITransMenuWnd};
const uintptr_t hierarchy_UITransMenuWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UITransMenuWnd};
const uintptr_t bases_UITransMenuWnd[] = {(uintptr_t)base_UITransMenuWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UITransMenuWnd[] = {(uintptr_t)&type_UITransMenuWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UITransMenuWnd};
const uintptr_t locator_UIShowWarpWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIShowWarpWnd, (uintptr_t)hierarchy_UIShowWarpWnd};
const uintptr_t hierarchy_UIShowWarpWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIShowWarpWnd};
const uintptr_t bases_UIShowWarpWnd[] = {(uintptr_t)base_UIShowWarpWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIShowWarpWnd[] = {(uintptr_t)&type_UIShowWarpWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIShowWarpWnd};
const uintptr_t locator_UIStaticText_LF[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIStaticText_LF, (uintptr_t)hierarchy_UIStaticText_LF};
const uintptr_t hierarchy_UIStaticText_LF[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIStaticText_LF};
const uintptr_t bases_UIStaticText_LF[] = {(uintptr_t)base_UIStaticText_LF, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIStaticText_LF[] = {(uintptr_t)&type_UIStaticText_LF, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIStaticText_LF};
const uintptr_t locator_UINoteWnd_Scroll[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINoteWnd_Scroll, (uintptr_t)hierarchy_UINoteWnd_Scroll};
const uintptr_t hierarchy_UINoteWnd_Scroll[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UINoteWnd_Scroll};
const uintptr_t bases_UINoteWnd_Scroll[] = {(uintptr_t)base_UINoteWnd_Scroll, (uintptr_t)base_UIScrollWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINoteWnd_Scroll[] = {(uintptr_t)&type_UINoteWnd_Scroll, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINoteWnd_Scroll};
const uintptr_t locator_UINoteWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINoteWnd, (uintptr_t)hierarchy_UINoteWnd};
const uintptr_t hierarchy_UINoteWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UINoteWnd};
const uintptr_t bases_UINoteWnd[] = {(uintptr_t)base_UINoteWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINoteWnd[] = {(uintptr_t)&type_UINoteWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINoteWnd};
const uintptr_t locator_UIRPImageWnd2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRPImageWnd2, (uintptr_t)hierarchy_UIRPImageWnd2};
const uintptr_t hierarchy_UIRPImageWnd2[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIRPImageWnd2};
const uintptr_t bases_UIRPImageWnd2[] = {(uintptr_t)base_UIRPImageWnd2, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRPImageWnd2[] = {(uintptr_t)&type_UIRPImageWnd2, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRPImageWnd2};
const uintptr_t locator_UIAccountLimitedToolWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIAccountLimitedToolWnd, (uintptr_t)hierarchy_UIAccountLimitedToolWnd};
const uintptr_t hierarchy_UIAccountLimitedToolWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIAccountLimitedToolWnd};
const uintptr_t bases_UIAccountLimitedToolWnd[] = {(uintptr_t)base_UIAccountLimitedToolWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIAccountLimitedToolWnd[] = {(uintptr_t)&type_UIAccountLimitedToolWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIAccountLimitedToolWnd};
const uintptr_t locator_UIHotkeyGuideWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIHotkeyGuideWnd, (uintptr_t)hierarchy_UIHotkeyGuideWnd};
const uintptr_t hierarchy_UIHotkeyGuideWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIHotkeyGuideWnd};
const uintptr_t bases_UIHotkeyGuideWnd[] = {(uintptr_t)base_UIHotkeyGuideWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIHotkeyGuideWnd[] = {(uintptr_t)&type_UIHotkeyGuideWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIHotkeyGuideWnd};
const uintptr_t locator_UIIllustWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIIllustWnd, (uintptr_t)hierarchy_UIIllustWnd};
const uintptr_t hierarchy_UIIllustWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIIllustWnd};
const uintptr_t bases_UIIllustWnd[] = {(uintptr_t)base_UIIllustWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIIllustWnd[] = {(uintptr_t)&type_UIIllustWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIIllustWnd};
const uintptr_t locator_UIKeyStrokeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIKeyStrokeWnd, (uintptr_t)hierarchy_UIKeyStrokeWnd};
const uintptr_t hierarchy_UIKeyStrokeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIKeyStrokeWnd};
const uintptr_t bases_UIKeyStrokeWnd[] = {(uintptr_t)base_UIKeyStrokeWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIKeyStrokeWnd[] = {(uintptr_t)&type_UIKeyStrokeWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIKeyStrokeWnd};
const uintptr_t locator_UICompositionWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UICompositionWnd, (uintptr_t)hierarchy_UICompositionWnd};
const uintptr_t hierarchy_UICompositionWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UICompositionWnd};
const uintptr_t bases_UICompositionWnd[] = {(uintptr_t)base_UICompositionWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UICompositionWnd[] = {(uintptr_t)&type_UICompositionWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UICompositionWnd};
const uintptr_t locator_UICandidateWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UICandidateWnd, (uintptr_t)hierarchy_UICandidateWnd};
const uintptr_t hierarchy_UICandidateWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UICandidateWnd};
const uintptr_t bases_UICandidateWnd[] = {(uintptr_t)base_UICandidateWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UICandidateWnd[] = {(uintptr_t)&type_UICandidateWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UICandidateWnd};
const uintptr_t locator_UIRoundBoxTooltipWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRoundBoxTooltipWnd, (uintptr_t)hierarchy_UIRoundBoxTooltipWnd};
const uintptr_t hierarchy_UIRoundBoxTooltipWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIRoundBoxTooltipWnd};
const uintptr_t bases_UIRoundBoxTooltipWnd[] = {(uintptr_t)base_UIRoundBoxTooltipWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRoundBoxTooltipWnd[] = {(uintptr_t)&type_UIRoundBoxTooltipWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRoundBoxTooltipWnd};
const uintptr_t locator_UIColoringTextViewer[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIColoringTextViewer, (uintptr_t)hierarchy_UIColoringTextViewer};
const uintptr_t hierarchy_UIColoringTextViewer[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIColoringTextViewer};
const uintptr_t bases_UIColoringTextViewer[] = {(uintptr_t)base_UIColoringTextViewer, (uintptr_t)base_UIListBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIColoringTextViewer[] = {(uintptr_t)&type_UIColoringTextViewer, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIColoringTextViewer};
const uintptr_t locator_UIMiniMapWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMiniMapWnd, (uintptr_t)hierarchy_UIMiniMapWnd};
const uintptr_t hierarchy_UIMiniMapWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMiniMapWnd};
const uintptr_t bases_UIMiniMapWnd[] = {(uintptr_t)base_UIMiniMapWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMiniMapWnd[] = {(uintptr_t)&type_UIMiniMapWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMiniMapWnd};
const uintptr_t locator_UIEditctrlWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEditctrlWnd, (uintptr_t)hierarchy_UIEditctrlWnd};
const uintptr_t hierarchy_UIEditctrlWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIEditctrlWnd};
const uintptr_t bases_UIEditctrlWnd[] = {(uintptr_t)base_UIEditctrlWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEditctrlWnd[] = {(uintptr_t)&type_UIEditctrlWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEditctrlWnd};
const uintptr_t locator_UIMerchantItemLogWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMerchantItemLogWnd, (uintptr_t)hierarchy_UIMerchantItemLogWnd};
const uintptr_t hierarchy_UIMerchantItemLogWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIMerchantItemLogWnd};
const uintptr_t bases_UIMerchantItemLogWnd[] = {(uintptr_t)base_UIMerchantItemLogWnd, (uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMerchantItemLogWnd[] = {(uintptr_t)&type_UIMerchantItemLogWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMerchantItemLogWnd};
const uintptr_t locator_UIMakeTargetListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMakeTargetListWnd, (uintptr_t)hierarchy_UIMakeTargetListWnd};
const uintptr_t hierarchy_UIMakeTargetListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMakeTargetListWnd};
const uintptr_t bases_UIMakeTargetListWnd[] = {(uintptr_t)base_UIMakeTargetListWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMakeTargetListWnd[] = {(uintptr_t)&type_UIMakeTargetListWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMakeTargetListWnd};
const uintptr_t locator_UIItemRepairWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIItemRepairWnd, (uintptr_t)hierarchy_UIItemRepairWnd};
const uintptr_t hierarchy_UIItemRepairWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIItemRepairWnd};
const uintptr_t bases_UIItemRepairWnd[] = {(uintptr_t)base_UIItemRepairWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIItemRepairWnd[] = {(uintptr_t)&type_UIItemRepairWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIItemRepairWnd};
const uintptr_t locator_UIWeaponRefineWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIWeaponRefineWnd, (uintptr_t)hierarchy_UIWeaponRefineWnd};
const uintptr_t hierarchy_UIWeaponRefineWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIWeaponRefineWnd};
const uintptr_t bases_UIWeaponRefineWnd[] = {(uintptr_t)base_UIWeaponRefineWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIWeaponRefineWnd[] = {(uintptr_t)&type_UIWeaponRefineWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIWeaponRefineWnd};
const uintptr_t locator_UIMakeTargetProcessWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMakeTargetProcessWnd, (uintptr_t)hierarchy_UIMakeTargetProcessWnd};
const uintptr_t hierarchy_UIMakeTargetProcessWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMakeTargetProcessWnd};
const uintptr_t bases_UIMakeTargetProcessWnd[] = {(uintptr_t)base_UIMakeTargetProcessWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMakeTargetProcessWnd[] = {(uintptr_t)&type_UIMakeTargetProcessWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMakeTargetProcessWnd};
const uintptr_t locator_UIMakeTargetResultWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMakeTargetResultWnd, (uintptr_t)hierarchy_UIMakeTargetResultWnd};
const uintptr_t hierarchy_UIMakeTargetResultWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMakeTargetResultWnd};
const uintptr_t bases_UIMakeTargetResultWnd[] = {(uintptr_t)base_UIMakeTargetResultWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMakeTargetResultWnd[] = {(uintptr_t)&type_UIMakeTargetResultWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMakeTargetResultWnd};
const uintptr_t locator_UIParaItemShopWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIParaItemShopWnd, (uintptr_t)hierarchy_UIParaItemShopWnd};
const uintptr_t hierarchy_UIParaItemShopWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_UIParaItemShopWnd};
const uintptr_t bases_UIParaItemShopWnd[] = {(uintptr_t)base_UIParaItemShopWnd, (uintptr_t)base_UIItemShopWnd, (uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIParaItemShopWnd[] = {(uintptr_t)&type_UIParaItemShopWnd, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIParaItemShopWnd};
const uintptr_t locator_UIParaItemPurchaseWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIParaItemPurchaseWnd, (uintptr_t)hierarchy_UIParaItemPurchaseWnd};
const uintptr_t hierarchy_UIParaItemPurchaseWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_UIParaItemPurchaseWnd};
const uintptr_t bases_UIParaItemPurchaseWnd[] = {(uintptr_t)base_UIParaItemPurchaseWnd, (uintptr_t)base_UIItemPurchaseWnd, (uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIParaItemPurchaseWnd[] = {(uintptr_t)&type_UIParaItemPurchaseWnd, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIParaItemPurchaseWnd};
const uintptr_t locator_UIParaResultWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIParaResultWnd, (uintptr_t)hierarchy_UIParaResultWnd};
const uintptr_t hierarchy_UIParaResultWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIParaResultWnd};
const uintptr_t bases_UIParaResultWnd[] = {(uintptr_t)base_UIParaResultWnd, (uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIParaResultWnd[] = {(uintptr_t)&type_UIParaResultWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIParaResultWnd};
const uintptr_t locator_UIPetEvolutionWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIPetEvolutionWnd, (uintptr_t)hierarchy_UIPetEvolutionWnd};
const uintptr_t hierarchy_UIPetEvolutionWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIPetEvolutionWnd};
const uintptr_t bases_UIPetEvolutionWnd[] = {(uintptr_t)base_UIPetEvolutionWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIPetEvolutionWnd[] = {(uintptr_t)&type_UIPetEvolutionWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIPetEvolutionWnd};
const uintptr_t locator_UIScrollWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIScrollWnd, (uintptr_t)hierarchy_UIScrollWnd};
const uintptr_t locator_UITagButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UITagButton, (uintptr_t)hierarchy_UITagButton};
const uintptr_t hierarchy_UITagButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UITagButton};
const uintptr_t bases_UITagButton[] = {(uintptr_t)base_UITagButton, (uintptr_t)base_UITextButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UITagButton[] = {(uintptr_t)&type_UITagButton, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UITagButton};
const uintptr_t locator_UITagListBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UITagListBox, (uintptr_t)hierarchy_UITagListBox};
const uintptr_t hierarchy_UITagListBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UITagListBox};
const uintptr_t bases_UITagListBox[] = {(uintptr_t)base_UITagListBox, (uintptr_t)base_UIListBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UITagListBox[] = {(uintptr_t)&type_UITagListBox, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UITagListBox};
const uintptr_t locator_UIViewCameraInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIViewCameraInfoWnd, (uintptr_t)hierarchy_UIViewCameraInfoWnd};
const uintptr_t hierarchy_UIViewCameraInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIViewCameraInfoWnd};
const uintptr_t bases_UIViewCameraInfoWnd[] = {(uintptr_t)base_UIViewCameraInfoWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIViewCameraInfoWnd[] = {(uintptr_t)&type_UIViewCameraInfoWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIViewCameraInfoWnd};
const uintptr_t locator_UIWindow[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIWindow, (uintptr_t)hierarchy_UIWindow};
const uintptr_t locator_UICashShopWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UICashShopWnd, (uintptr_t)hierarchy_UICashShopWnd};
const uintptr_t hierarchy_UICashShopWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UICashShopWnd};
const uintptr_t bases_UICashShopWnd[] = {(uintptr_t)base_UICashShopWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UICashShopWnd[] = {(uintptr_t)&type_UICashShopWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UICashShopWnd};
const uintptr_t locator_UICashShopWnd_Banner[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UICashShopWnd_Banner, (uintptr_t)hierarchy_UICashShopWnd_Banner};
const uintptr_t hierarchy_UICashShopWnd_Banner[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UICashShopWnd_Banner};
const uintptr_t bases_UICashShopWnd_Banner[] = {(uintptr_t)base_UICashShopWnd_Banner, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UICashShopWnd_Banner[] = {(uintptr_t)&type_UICashShopWnd_Banner, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UICashShopWnd_Banner};
const uintptr_t locator_UICashShopWnd_CartItem[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UICashShopWnd_CartItem, (uintptr_t)hierarchy_UICashShopWnd_CartItem};
const uintptr_t hierarchy_UICashShopWnd_CartItem[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UICashShopWnd_CartItem};
const uintptr_t bases_UICashShopWnd_CartItem[] = {(uintptr_t)base_UICashShopWnd_CartItem, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UICashShopWnd_CartItem[] = {(uintptr_t)&type_UICashShopWnd_CartItem, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UICashShopWnd_CartItem};
const uintptr_t locator_UICashShopWnd_Cart[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UICashShopWnd_Cart, (uintptr_t)hierarchy_UICashShopWnd_Cart};
const uintptr_t hierarchy_UICashShopWnd_Cart[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UICashShopWnd_Cart};
const uintptr_t bases_UICashShopWnd_Cart[] = {(uintptr_t)base_UICashShopWnd_Cart, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UICashShopWnd_Cart[] = {(uintptr_t)&type_UICashShopWnd_Cart, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UICashShopWnd_Cart};
const uintptr_t locator_UICashShopWnd_Item[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UICashShopWnd_Item, (uintptr_t)hierarchy_UICashShopWnd_Item};
const uintptr_t hierarchy_UICashShopWnd_Item[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UICashShopWnd_Item};
const uintptr_t bases_UICashShopWnd_Item[] = {(uintptr_t)base_UICashShopWnd_Item, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UICashShopWnd_Item[] = {(uintptr_t)&type_UICashShopWnd_Item, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UICashShopWnd_Item};
const uintptr_t locator_UINewSelectCharWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINewSelectCharWnd, (uintptr_t)hierarchy_UINewSelectCharWnd};
const uintptr_t hierarchy_UINewSelectCharWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UINewSelectCharWnd};
const uintptr_t bases_UINewSelectCharWnd[] = {(uintptr_t)base_UINewSelectCharWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINewSelectCharWnd[] = {(uintptr_t)&type_UINewSelectCharWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINewSelectCharWnd};
const uintptr_t locator_UIHairBitmap[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIHairBitmap, (uintptr_t)hierarchy_UIHairBitmap};
const uintptr_t hierarchy_UIHairBitmap[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIHairBitmap};
const uintptr_t bases_UIHairBitmap[] = {(uintptr_t)base_UIHairBitmap, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIHairBitmap[] = {(uintptr_t)&type_UIHairBitmap, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIHairBitmap};
const uintptr_t locator_UIHairStyleButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIHairStyleButton, (uintptr_t)hierarchy_UIHairStyleButton};
const uintptr_t hierarchy_UIHairStyleButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIHairStyleButton};
const uintptr_t bases_UIHairStyleButton[] = {(uintptr_t)base_UIHairStyleButton, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIHairStyleButton[] = {(uintptr_t)&type_UIHairStyleButton, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIHairStyleButton};
const uintptr_t locator_UINewMakeCharWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINewMakeCharWnd, (uintptr_t)hierarchy_UINewMakeCharWnd};
const uintptr_t hierarchy_UINewMakeCharWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UINewMakeCharWnd};
const uintptr_t bases_UINewMakeCharWnd[] = {(uintptr_t)base_UINewMakeCharWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINewMakeCharWnd[] = {(uintptr_t)&type_UINewMakeCharWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINewMakeCharWnd};
const uintptr_t locator_UICheckAttendanceWnd_Banner[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UICheckAttendanceWnd_Banner, (uintptr_t)hierarchy_UICheckAttendanceWnd_Banner};
const uintptr_t hierarchy_UICheckAttendanceWnd_Banner[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UICheckAttendanceWnd_Banner};
const uintptr_t bases_UICheckAttendanceWnd_Banner[] = {(uintptr_t)base_UICheckAttendanceWnd_Banner, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UICheckAttendanceWnd_Banner[] = {(uintptr_t)&type_UICheckAttendanceWnd_Banner, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UICheckAttendanceWnd_Banner};
const uintptr_t locator_UICheckAttendanceWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UICheckAttendanceWnd, (uintptr_t)hierarchy_UICheckAttendanceWnd};
const uintptr_t hierarchy_UICheckAttendanceWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UICheckAttendanceWnd};
const uintptr_t bases_UICheckAttendanceWnd[] = {(uintptr_t)base_UICheckAttendanceWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UICheckAttendanceWnd[] = {(uintptr_t)&type_UICheckAttendanceWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UICheckAttendanceWnd};
const uintptr_t locator_CChoicePackageItemBoxWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CChoicePackageItemBoxWnd, (uintptr_t)hierarchy_CChoicePackageItemBoxWnd};
const uintptr_t hierarchy_CChoicePackageItemBoxWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_CChoicePackageItemBoxWnd};
const uintptr_t bases_CChoicePackageItemBoxWnd[] = {(uintptr_t)base_CChoicePackageItemBoxWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_CChoicePackageItemBoxWnd[] = {(uintptr_t)&type_CChoicePackageItemBoxWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CChoicePackageItemBoxWnd};
const uintptr_t locator_UIControlRectWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIControlRectWnd, (uintptr_t)hierarchy_UIControlRectWnd};
const uintptr_t hierarchy_UIControlRectWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIControlRectWnd};
const uintptr_t bases_UIControlRectWnd[] = {(uintptr_t)base_UIControlRectWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIControlRectWnd[] = {(uintptr_t)&type_UIControlRectWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIControlRectWnd};
const uintptr_t locator_UIConnectTwitterWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIConnectTwitterWnd, (uintptr_t)hierarchy_UIConnectTwitterWnd};
const uintptr_t hierarchy_UIConnectTwitterWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIConnectTwitterWnd};
const uintptr_t bases_UIConnectTwitterWnd[] = {(uintptr_t)base_UIConnectTwitterWnd, (uintptr_t)base_UIConnectSnsWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIConnectTwitterWnd[] = {(uintptr_t)&type_UIConnectTwitterWnd, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIConnectTwitterWnd};
const uintptr_t base_UIConnectSnsWnd[] = {(uintptr_t)&type_UIConnectSnsWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIConnectSnsWnd};
const uintptr_t hierarchy_UIConnectSnsWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIConnectSnsWnd};
const uintptr_t bases_UIConnectSnsWnd[] = {(uintptr_t)base_UIConnectSnsWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t locator_UIConnectSnsWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIConnectSnsWnd, (uintptr_t)hierarchy_UIConnectSnsWnd};
const uintptr_t locator_UITwitterScrollBar[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UITwitterScrollBar, (uintptr_t)hierarchy_UITwitterScrollBar};
const uintptr_t hierarchy_UITwitterScrollBar[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UITwitterScrollBar};
const uintptr_t bases_UITwitterScrollBar[] = {(uintptr_t)base_UITwitterScrollBar, (uintptr_t)base_UIScrollBar, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UITwitterScrollBar[] = {(uintptr_t)&type_UITwitterScrollBar, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UITwitterScrollBar};
const uintptr_t locator_UITwitterEditCtrl3[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UITwitterEditCtrl3, (uintptr_t)hierarchy_UITwitterEditCtrl3};
const uintptr_t hierarchy_UITwitterEditCtrl3[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_UITwitterEditCtrl3};
const uintptr_t bases_UITwitterEditCtrl3[] = {(uintptr_t)base_UITwitterEditCtrl3, (uintptr_t)base_UIEditCtrl3, (uintptr_t)base_UIEditCtrl2, (uintptr_t)base_UIEditCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UITwitterEditCtrl3[] = {(uintptr_t)&type_UITwitterEditCtrl3, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UITwitterEditCtrl3};
const uintptr_t locator_UIGradeEnchant_BtnStart[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIGradeEnchant_BtnStart, (uintptr_t)hierarchy_UIGradeEnchant_BtnStart};
const uintptr_t hierarchy_UIGradeEnchant_BtnStart[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIGradeEnchant_BtnStart};
const uintptr_t bases_UIGradeEnchant_BtnStart[] = {(uintptr_t)base_UIGradeEnchant_BtnStart, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIGradeEnchant_BtnStart[] = {(uintptr_t)&type_UIGradeEnchant_BtnStart, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIGradeEnchant_BtnStart};
const uintptr_t locator_UIGradeEnchant_BtnEnhancedUp[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIGradeEnchant_BtnEnhancedUp, (uintptr_t)hierarchy_UIGradeEnchant_BtnEnhancedUp};
const uintptr_t hierarchy_UIGradeEnchant_BtnEnhancedUp[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIGradeEnchant_BtnEnhancedUp};
const uintptr_t bases_UIGradeEnchant_BtnEnhancedUp[] = {(uintptr_t)base_UIGradeEnchant_BtnEnhancedUp, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIGradeEnchant_BtnEnhancedUp[] = {(uintptr_t)&type_UIGradeEnchant_BtnEnhancedUp, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIGradeEnchant_BtnEnhancedUp};
const uintptr_t locator_UIGradeEnchant_BtnEnhancedDown[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIGradeEnchant_BtnEnhancedDown, (uintptr_t)hierarchy_UIGradeEnchant_BtnEnhancedDown};
const uintptr_t hierarchy_UIGradeEnchant_BtnEnhancedDown[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIGradeEnchant_BtnEnhancedDown};
const uintptr_t bases_UIGradeEnchant_BtnEnhancedDown[] = {(uintptr_t)base_UIGradeEnchant_BtnEnhancedDown, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIGradeEnchant_BtnEnhancedDown[] = {(uintptr_t)&type_UIGradeEnchant_BtnEnhancedDown, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIGradeEnchant_BtnEnhancedDown};
const uintptr_t locator_UIGradeEnchantWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIGradeEnchantWnd, (uintptr_t)hierarchy_UIGradeEnchantWnd};
const uintptr_t hierarchy_UIGradeEnchantWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIGradeEnchantWnd};
const uintptr_t bases_UIGradeEnchantWnd[] = {(uintptr_t)base_UIGradeEnchantWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIGradeEnchantWnd[] = {(uintptr_t)&type_UIGradeEnchantWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIGradeEnchantWnd};
const uintptr_t locator_UIGraphicSettingWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIGraphicSettingWnd, (uintptr_t)hierarchy_UIGraphicSettingWnd};
const uintptr_t hierarchy_UIGraphicSettingWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIGraphicSettingWnd};
const uintptr_t bases_UIGraphicSettingWnd[] = {(uintptr_t)base_UIGraphicSettingWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIGraphicSettingWnd[] = {(uintptr_t)&type_UIGraphicSettingWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIGraphicSettingWnd};
const uintptr_t locator_UICaptchaRegisterWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UICaptchaRegisterWnd, (uintptr_t)hierarchy_UICaptchaRegisterWnd};
const uintptr_t hierarchy_UICaptchaRegisterWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UICaptchaRegisterWnd};
const uintptr_t bases_UICaptchaRegisterWnd[] = {(uintptr_t)base_UICaptchaRegisterWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UICaptchaRegisterWnd[] = {(uintptr_t)&type_UICaptchaRegisterWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UICaptchaRegisterWnd};
const uintptr_t locator_UIMacroDetectorWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMacroDetectorWnd, (uintptr_t)hierarchy_UIMacroDetectorWnd};
const uintptr_t hierarchy_UIMacroDetectorWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMacroDetectorWnd};
const uintptr_t bases_UIMacroDetectorWnd[] = {(uintptr_t)base_UIMacroDetectorWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMacroDetectorWnd[] = {(uintptr_t)&type_UIMacroDetectorWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMacroDetectorWnd};
const uintptr_t locator_UICaptchaPreviewWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UICaptchaPreviewWnd, (uintptr_t)hierarchy_UICaptchaPreviewWnd};
const uintptr_t hierarchy_UICaptchaPreviewWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UICaptchaPreviewWnd};
const uintptr_t bases_UICaptchaPreviewWnd[] = {(uintptr_t)base_UICaptchaPreviewWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UICaptchaPreviewWnd[] = {(uintptr_t)&type_UICaptchaPreviewWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UICaptchaPreviewWnd};
const uintptr_t locator_UIMacroReporterWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMacroReporterWnd, (uintptr_t)hierarchy_UIMacroReporterWnd};
const uintptr_t hierarchy_UIMacroReporterWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMacroReporterWnd};
const uintptr_t bases_UIMacroReporterWnd[] = {(uintptr_t)base_UIMacroReporterWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMacroReporterWnd[] = {(uintptr_t)&type_UIMacroReporterWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMacroReporterWnd};
const uintptr_t locator_UIMacroUserInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMacroUserInfoWnd, (uintptr_t)hierarchy_UIMacroUserInfoWnd};
const uintptr_t hierarchy_UIMacroUserInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMacroUserInfoWnd};
const uintptr_t bases_UIMacroUserInfoWnd[] = {(uintptr_t)base_UIMacroUserInfoWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMacroUserInfoWnd[] = {(uintptr_t)&type_UIMacroUserInfoWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMacroUserInfoWnd};
const uintptr_t locator_UIPreviewEquipWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIPreviewEquipWnd, (uintptr_t)hierarchy_UIPreviewEquipWnd};
const uintptr_t hierarchy_UIPreviewEquipWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIPreviewEquipWnd};
const uintptr_t bases_UIPreviewEquipWnd[] = {(uintptr_t)base_UIPreviewEquipWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIPreviewEquipWnd[] = {(uintptr_t)&type_UIPreviewEquipWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIPreviewEquipWnd};
const uintptr_t locator_UIRefining_BtnRefining[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRefining_BtnRefining, (uintptr_t)hierarchy_UIRefining_BtnRefining};
const uintptr_t hierarchy_UIRefining_BtnRefining[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIRefining_BtnRefining};
const uintptr_t bases_UIRefining_BtnRefining[] = {(uintptr_t)base_UIRefining_BtnRefining, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRefining_BtnRefining[] = {(uintptr_t)&type_UIRefining_BtnRefining, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRefining_BtnRefining};
const uintptr_t locator_UIRefining_BtnReturn[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRefining_BtnReturn, (uintptr_t)hierarchy_UIRefining_BtnReturn};
const uintptr_t hierarchy_UIRefining_BtnReturn[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIRefining_BtnReturn};
const uintptr_t bases_UIRefining_BtnReturn[] = {(uintptr_t)base_UIRefining_BtnReturn, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRefining_BtnReturn[] = {(uintptr_t)&type_UIRefining_BtnReturn, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRefining_BtnReturn};
const uintptr_t locator_UIRefining_BtnAgain[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRefining_BtnAgain, (uintptr_t)hierarchy_UIRefining_BtnAgain};
const uintptr_t hierarchy_UIRefining_BtnAgain[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIRefining_BtnAgain};
const uintptr_t bases_UIRefining_BtnAgain[] = {(uintptr_t)base_UIRefining_BtnAgain, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRefining_BtnAgain[] = {(uintptr_t)&type_UIRefining_BtnAgain, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRefining_BtnAgain};
const uintptr_t locator_UIRefining_Notice[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRefining_Notice, (uintptr_t)hierarchy_UIRefining_Notice};
const uintptr_t hierarchy_UIRefining_Notice[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIRefining_Notice};
const uintptr_t bases_UIRefining_Notice[] = {(uintptr_t)base_UIRefining_Notice, (uintptr_t)base_UIStaticText, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRefining_Notice[] = {(uintptr_t)&type_UIRefining_Notice, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRefining_Notice};
const uintptr_t locator_UIRefiningWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRefiningWnd, (uintptr_t)hierarchy_UIRefiningWnd};
const uintptr_t hierarchy_UIRefiningWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIRefiningWnd};
const uintptr_t bases_UIRefiningWnd[] = {(uintptr_t)base_UIRefiningWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRefiningWnd[] = {(uintptr_t)&type_UIRefiningWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRefiningWnd};
const uintptr_t locator_UINpcSprWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINpcSprWnd, (uintptr_t)hierarchy_UINpcSprWnd};
const uintptr_t hierarchy_UINpcSprWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UINpcSprWnd};
const uintptr_t bases_UINpcSprWnd[] = {(uintptr_t)base_UINpcSprWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINpcSprWnd[] = {(uintptr_t)&type_UINpcSprWnd, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINpcSprWnd};
const uintptr_t locator_UIOngoingQuestScrollWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIOngoingQuestScrollWnd, (uintptr_t)hierarchy_UIOngoingQuestScrollWnd};
const uintptr_t hierarchy_UIOngoingQuestScrollWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIOngoingQuestScrollWnd};
const uintptr_t bases_UIOngoingQuestScrollWnd[] = {(uintptr_t)base_UIOngoingQuestScrollWnd, (uintptr_t)base_UIScrollWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIOngoingQuestScrollWnd[] = {(uintptr_t)&type_UIOngoingQuestScrollWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIOngoingQuestScrollWnd};
const uintptr_t locator_UIRecommededQuestScrollWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRecommededQuestScrollWnd, (uintptr_t)hierarchy_UIRecommededQuestScrollWnd};
const uintptr_t hierarchy_UIRecommededQuestScrollWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIRecommededQuestScrollWnd};
const uintptr_t bases_UIRecommededQuestScrollWnd[] = {(uintptr_t)base_UIRecommededQuestScrollWnd, (uintptr_t)base_UIScrollWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRecommededQuestScrollWnd[] = {(uintptr_t)&type_UIRecommededQuestScrollWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRecommededQuestScrollWnd};
const uintptr_t locator_UIRenewDetailQuestInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRenewDetailQuestInfoWnd, (uintptr_t)hierarchy_UIRenewDetailQuestInfoWnd};
const uintptr_t hierarchy_UIRenewDetailQuestInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIRenewDetailQuestInfoWnd};
const uintptr_t bases_UIRenewDetailQuestInfoWnd[] = {(uintptr_t)base_UIRenewDetailQuestInfoWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRenewDetailQuestInfoWnd[] = {(uintptr_t)&type_UIRenewDetailQuestInfoWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRenewDetailQuestInfoWnd};
const uintptr_t locator_UIRenewQuestScrollBar[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRenewQuestScrollBar, (uintptr_t)hierarchy_UIRenewQuestScrollBar};
const uintptr_t hierarchy_UIRenewQuestScrollBar[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIRenewQuestScrollBar};
const uintptr_t bases_UIRenewQuestScrollBar[] = {(uintptr_t)base_UIRenewQuestScrollBar, (uintptr_t)base_UIScrollBar, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRenewQuestScrollBar[] = {(uintptr_t)&type_UIRenewQuestScrollBar, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRenewQuestScrollBar};
const uintptr_t locator_UIRenewQuestWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRenewQuestWnd, (uintptr_t)hierarchy_UIRenewQuestWnd};
const uintptr_t hierarchy_UIRenewQuestWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIRenewQuestWnd};
const uintptr_t bases_UIRenewQuestWnd[] = {(uintptr_t)base_UIRenewQuestWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRenewQuestWnd[] = {(uintptr_t)&type_UIRenewQuestWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRenewQuestWnd};
const uintptr_t locator_UIRodexScrollBar[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRodexScrollBar, (uintptr_t)hierarchy_UIRodexScrollBar};
const uintptr_t hierarchy_UIRodexScrollBar[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIRodexScrollBar};
const uintptr_t bases_UIRodexScrollBar[] = {(uintptr_t)base_UIRodexScrollBar, (uintptr_t)base_UIScrollBar, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRodexScrollBar[] = {(uintptr_t)&type_UIRodexScrollBar, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRodexScrollBar};
const uintptr_t locator_UIRodexEditCtrl3[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRodexEditCtrl3, (uintptr_t)hierarchy_UIRodexEditCtrl3};
const uintptr_t hierarchy_UIRodexEditCtrl3[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_UIRodexEditCtrl3};
const uintptr_t bases_UIRodexEditCtrl3[] = {(uintptr_t)base_UIRodexEditCtrl3, (uintptr_t)base_UIEditCtrl3, (uintptr_t)base_UIEditCtrl2, (uintptr_t)base_UIEditCtrl, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRodexEditCtrl3[] = {(uintptr_t)&type_UIRodexEditCtrl3, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRodexEditCtrl3};
const uintptr_t locator_UIRodexComposeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRodexComposeWnd, (uintptr_t)hierarchy_UIRodexComposeWnd};
const uintptr_t hierarchy_UIRodexComposeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIRodexComposeWnd};
const uintptr_t bases_UIRodexComposeWnd[] = {(uintptr_t)base_UIRodexComposeWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRodexComposeWnd[] = {(uintptr_t)&type_UIRodexComposeWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRodexComposeWnd};
const uintptr_t locator_UIRodexContentsWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRodexContentsWnd, (uintptr_t)hierarchy_UIRodexContentsWnd};
const uintptr_t hierarchy_UIRodexContentsWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIRodexContentsWnd};
const uintptr_t bases_UIRodexContentsWnd[] = {(uintptr_t)base_UIRodexContentsWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRodexContentsWnd[] = {(uintptr_t)&type_UIRodexContentsWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRodexContentsWnd};
const uintptr_t locator_UIRodexInboxWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRodexInboxWnd, (uintptr_t)hierarchy_UIRodexInboxWnd};
const uintptr_t hierarchy_UIRodexInboxWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)6, (uintptr_t)bases_UIRodexInboxWnd};
const uintptr_t bases_UIRodexInboxWnd[] = {(uintptr_t)base_UIRodexInboxWnd, (uintptr_t)base_UIItemWnd, (uintptr_t)base_UIItemBaseWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRodexInboxWnd[] = {(uintptr_t)&type_UIRodexInboxWnd, (uintptr_t)5, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRodexInboxWnd};
const uintptr_t locator_UIRoulletteWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRoulletteWnd, (uintptr_t)hierarchy_UIRoulletteWnd};
const uintptr_t hierarchy_UIRoulletteWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIRoulletteWnd};
const uintptr_t bases_UIRoulletteWnd[] = {(uintptr_t)base_UIRoulletteWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRoulletteWnd[] = {(uintptr_t)&type_UIRoulletteWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRoulletteWnd};
const uintptr_t locator_UIRenderTargetWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIRenderTargetWnd, (uintptr_t)hierarchy_UIRenderTargetWnd};
const uintptr_t hierarchy_UIRenderTargetWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIRenderTargetWnd};
const uintptr_t bases_UIRenderTargetWnd[] = {(uintptr_t)base_UIRenderTargetWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIRenderTargetWnd[] = {(uintptr_t)&type_UIRenderTargetWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIRenderTargetWnd};
const uintptr_t locator_CIcon[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CIcon, (uintptr_t)hierarchy_CIcon};
const uintptr_t hierarchy_CIcon[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_CIcon};
const uintptr_t bases_CIcon[] = {(uintptr_t)base_CIcon, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_CIcon[] = {(uintptr_t)&type_CIcon, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CIcon};
const uintptr_t locator_CBmpObjWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CBmpObjWnd, (uintptr_t)hierarchy_CBmpObjWnd};
const uintptr_t hierarchy_CBmpObjWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_CBmpObjWnd};
const uintptr_t bases_CBmpObjWnd[] = {(uintptr_t)base_CBmpObjWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_CBmpObjWnd[] = {(uintptr_t)&type_CBmpObjWnd, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CBmpObjWnd};
const uintptr_t locator_CSignBoardWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_CSignBoardWnd, (uintptr_t)hierarchy_CSignBoardWnd};
const uintptr_t hierarchy_CSignBoardWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_CSignBoardWnd};
const uintptr_t bases_CSignBoardWnd[] = {(uintptr_t)base_CSignBoardWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_CSignBoardWnd[] = {(uintptr_t)&type_CSignBoardWnd, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_CSignBoardWnd};
const uintptr_t locator_C3DCooRendering[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_C3DCooRendering, (uintptr_t)hierarchy_C3DCooRendering};
const uintptr_t hierarchy_C3DCooRendering[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_C3DCooRendering};
const uintptr_t bases_C3DCooRendering[] = {(uintptr_t)base_C3DCooRendering, (uintptr_t)base_CRenderObject, (uintptr_t)base_CGameObject};
const uintptr_t base_C3DCooRendering[] = {(uintptr_t)&type_C3DCooRendering, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_C3DCooRendering};
const uintptr_t locator_UIDisplayCharacter[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIDisplayCharacter, (uintptr_t)hierarchy_UIDisplayCharacter};
const uintptr_t hierarchy_UIDisplayCharacter[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIDisplayCharacter};
const uintptr_t bases_UIDisplayCharacter[] = {(uintptr_t)base_UIDisplayCharacter, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIDisplayCharacter[] = {(uintptr_t)&type_UIDisplayCharacter, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIDisplayCharacter};
const uintptr_t locator_UIStylingShopButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIStylingShopButton, (uintptr_t)hierarchy_UIStylingShopButton};
const uintptr_t hierarchy_UIStylingShopButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIStylingShopButton};
const uintptr_t bases_UIStylingShopButton[] = {(uintptr_t)base_UIStylingShopButton, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIStylingShopButton[] = {(uintptr_t)&type_UIStylingShopButton, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIStylingShopButton};
const uintptr_t locator_UIAccBitmapButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIAccBitmapButton, (uintptr_t)hierarchy_UIAccBitmapButton};
const uintptr_t hierarchy_UIAccBitmapButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UIAccBitmapButton};
const uintptr_t bases_UIAccBitmapButton[] = {(uintptr_t)base_UIAccBitmapButton, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIAccBitmapButton[] = {(uintptr_t)&type_UIAccBitmapButton, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIAccBitmapButton};
const uintptr_t locator_UI2ndCostumeCheckBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UI2ndCostumeCheckBox, (uintptr_t)hierarchy_UI2ndCostumeCheckBox};
const uintptr_t hierarchy_UI2ndCostumeCheckBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UI2ndCostumeCheckBox};
const uintptr_t bases_UI2ndCostumeCheckBox[] = {(uintptr_t)base_UI2ndCostumeCheckBox, (uintptr_t)base_UICheckBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UI2ndCostumeCheckBox[] = {(uintptr_t)&type_UI2ndCostumeCheckBox, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UI2ndCostumeCheckBox};
const uintptr_t locator_UIShowInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIShowInfoWnd, (uintptr_t)hierarchy_UIShowInfoWnd};
const uintptr_t hierarchy_UIShowInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIShowInfoWnd};
const uintptr_t bases_UIShowInfoWnd[] = {(uintptr_t)base_UIShowInfoWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIShowInfoWnd[] = {(uintptr_t)&type_UIShowInfoWnd, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIShowInfoWnd};
const uintptr_t locator_UIStylingshopTabParentWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIStylingshopTabParentWnd, (uintptr_t)hierarchy_UIStylingshopTabParentWnd};
const uintptr_t hierarchy_UIStylingshopTabParentWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIStylingshopTabParentWnd};
const uintptr_t bases_UIStylingshopTabParentWnd[] = {(uintptr_t)base_UIStylingshopTabParentWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIStylingshopTabParentWnd[] = {(uintptr_t)&type_UIStylingshopTabParentWnd, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIStylingshopTabParentWnd};
const uintptr_t locator_UIBasicStylingWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBasicStylingWnd, (uintptr_t)hierarchy_UIBasicStylingWnd};
const uintptr_t hierarchy_UIBasicStylingWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIBasicStylingWnd};
const uintptr_t bases_UIBasicStylingWnd[] = {(uintptr_t)base_UIBasicStylingWnd, (uintptr_t)base_UIStylingshopTabParentWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBasicStylingWnd[] = {(uintptr_t)&type_UIBasicStylingWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBasicStylingWnd};
const uintptr_t locator_UIAccessoryCusWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIAccessoryCusWnd, (uintptr_t)hierarchy_UIAccessoryCusWnd};
const uintptr_t hierarchy_UIAccessoryCusWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIAccessoryCusWnd};
const uintptr_t bases_UIAccessoryCusWnd[] = {(uintptr_t)base_UIAccessoryCusWnd, (uintptr_t)base_UIStylingshopTabParentWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIAccessoryCusWnd[] = {(uintptr_t)&type_UIAccessoryCusWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIAccessoryCusWnd};
const uintptr_t locator_UIAccessoryShowCase[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIAccessoryShowCase, (uintptr_t)hierarchy_UIAccessoryShowCase};
const uintptr_t hierarchy_UIAccessoryShowCase[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIAccessoryShowCase};
const uintptr_t bases_UIAccessoryShowCase[] = {(uintptr_t)base_UIAccessoryShowCase, (uintptr_t)base_UIScrollWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIAccessoryShowCase[] = {(uintptr_t)&type_UIAccessoryShowCase, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIAccessoryShowCase};
const uintptr_t locator_UI2ndCostumeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UI2ndCostumeWnd, (uintptr_t)hierarchy_UI2ndCostumeWnd};
const uintptr_t hierarchy_UI2ndCostumeWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UI2ndCostumeWnd};
const uintptr_t bases_UI2ndCostumeWnd[] = {(uintptr_t)base_UI2ndCostumeWnd, (uintptr_t)base_UIStylingshopTabParentWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UI2ndCostumeWnd[] = {(uintptr_t)&type_UI2ndCostumeWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UI2ndCostumeWnd};
const uintptr_t locator_UISlotOfMyStyleWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISlotOfMyStyleWnd, (uintptr_t)hierarchy_UISlotOfMyStyleWnd};
const uintptr_t hierarchy_UISlotOfMyStyleWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UISlotOfMyStyleWnd};
const uintptr_t bases_UISlotOfMyStyleWnd[] = {(uintptr_t)base_UISlotOfMyStyleWnd, (uintptr_t)base_UIStylingshopTabParentWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISlotOfMyStyleWnd[] = {(uintptr_t)&type_UISlotOfMyStyleWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISlotOfMyStyleWnd};
const uintptr_t locator_UISlotForSelectWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISlotForSelectWnd, (uintptr_t)hierarchy_UISlotForSelectWnd};
const uintptr_t hierarchy_UISlotForSelectWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UISlotForSelectWnd};
const uintptr_t bases_UISlotForSelectWnd[] = {(uintptr_t)base_UISlotForSelectWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISlotForSelectWnd[] = {(uintptr_t)&type_UISlotForSelectWnd, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISlotForSelectWnd};
const uintptr_t locator_UIMyStyleWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIMyStyleWnd, (uintptr_t)hierarchy_UIMyStyleWnd};
const uintptr_t hierarchy_UIMyStyleWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIMyStyleWnd};
const uintptr_t bases_UIMyStyleWnd[] = {(uintptr_t)base_UIMyStyleWnd, (uintptr_t)base_UIStylingshopTabParentWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIMyStyleWnd[] = {(uintptr_t)&type_UIMyStyleWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIMyStyleWnd};
const uintptr_t locator_UIStylingShopWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIStylingShopWnd, (uintptr_t)hierarchy_UIStylingShopWnd};
const uintptr_t hierarchy_UIStylingShopWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIStylingShopWnd};
const uintptr_t bases_UIStylingShopWnd[] = {(uintptr_t)base_UIStylingShopWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIStylingShopWnd[] = {(uintptr_t)&type_UIStylingShopWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIStylingShopWnd};
const uintptr_t locator_UISwapEquipmentWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISwapEquipmentWnd, (uintptr_t)hierarchy_UISwapEquipmentWnd};
const uintptr_t hierarchy_UISwapEquipmentWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UISwapEquipmentWnd};
const uintptr_t bases_UISwapEquipmentWnd[] = {(uintptr_t)base_UISwapEquipmentWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISwapEquipmentWnd[] = {(uintptr_t)&type_UISwapEquipmentWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISwapEquipmentWnd};
const uintptr_t locator_UISwapBitmapButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISwapBitmapButton, (uintptr_t)hierarchy_UISwapBitmapButton};
const uintptr_t hierarchy_UISwapBitmapButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)5, (uintptr_t)bases_UISwapBitmapButton};
const uintptr_t bases_UISwapBitmapButton[] = {(uintptr_t)base_UISwapBitmapButton, (uintptr_t)base_UIBitmapButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISwapBitmapButton[] = {(uintptr_t)&type_UISwapBitmapButton, (uintptr_t)4, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISwapBitmapButton};
const uintptr_t locator_LineCutter_UIText[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_LineCutter_UIText, (uintptr_t)hierarchy_LineCutter_UIText};
const uintptr_t hierarchy_LineCutter_UIText[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_LineCutter_UIText};
const uintptr_t bases_LineCutter_UIText[] = {(uintptr_t)base_LineCutter_UIText, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_LineCutter_UIText[] = {(uintptr_t)&type_LineCutter_UIText, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_LineCutter_UIText};
const uintptr_t locator_TagTextViewer_UIScroll[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_TagTextViewer_UIScroll, (uintptr_t)hierarchy_TagTextViewer_UIScroll};
const uintptr_t hierarchy_TagTextViewer_UIScroll[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_TagTextViewer_UIScroll};
const uintptr_t bases_TagTextViewer_UIScroll[] = {(uintptr_t)base_TagTextViewer_UIScroll, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_TagTextViewer_UIScroll[] = {(uintptr_t)&type_TagTextViewer_UIScroll, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_TagTextViewer_UIScroll};
const uintptr_t locator_UITagTextViewer[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UITagTextViewer, (uintptr_t)hierarchy_UITagTextViewer};
const uintptr_t hierarchy_UITagTextViewer[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UITagTextViewer};
const uintptr_t bases_UITagTextViewer[] = {(uintptr_t)base_UITagTextViewer, (uintptr_t)base_TagTextViewer_UIScroll, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UITagTextViewer[] = {(uintptr_t)&type_UITagTextViewer, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UITagTextViewer};
const uintptr_t locator_UIBarGraphPlayerAP[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBarGraphPlayerAP, (uintptr_t)hierarchy_UIBarGraphPlayerAP};
const uintptr_t hierarchy_UIBarGraphPlayerAP[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIBarGraphPlayerAP};
const uintptr_t bases_UIBarGraphPlayerAP[] = {(uintptr_t)base_UIBarGraphPlayerAP, (uintptr_t)base_UIBarGraphPlayer, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBarGraphPlayerAP[] = {(uintptr_t)&type_UIBarGraphPlayerAP, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBarGraphPlayerAP};
const uintptr_t locator_UIBasicInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIBasicInfoWnd, (uintptr_t)hierarchy_UIBasicInfoWnd};
const uintptr_t hierarchy_UIBasicInfoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIBasicInfoWnd};
const uintptr_t bases_UIBasicInfoWnd[] = {(uintptr_t)base_UIBasicInfoWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIBasicInfoWnd[] = {(uintptr_t)&type_UIBasicInfoWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIBasicInfoWnd};
const uintptr_t locator_UINewSkillListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UINewSkillListWnd, (uintptr_t)hierarchy_UINewSkillListWnd};
const uintptr_t hierarchy_UINewSkillListWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UINewSkillListWnd};
const uintptr_t bases_UINewSkillListWnd[] = {(uintptr_t)base_UINewSkillListWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UINewSkillListWnd[] = {(uintptr_t)&type_UINewSkillListWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UINewSkillListWnd};
const uintptr_t locator_UISkipButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UISkipButton, (uintptr_t)hierarchy_UISkipButton};
const uintptr_t hierarchy_UISkipButton[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UISkipButton};
const uintptr_t bases_UISkipButton[] = {(uintptr_t)base_UISkipButton, (uintptr_t)base_UIButton, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UISkipButton[] = {(uintptr_t)&type_UISkipButton, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UISkipButton};
const uintptr_t locator_UIVideoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIVideoWnd, (uintptr_t)hierarchy_UIVideoWnd};
const uintptr_t hierarchy_UIVideoWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)3, (uintptr_t)bases_UIVideoWnd};
const uintptr_t bases_UIVideoWnd[] = {(uintptr_t)base_UIVideoWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIVideoWnd[] = {(uintptr_t)&type_UIVideoWnd, (uintptr_t)2, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIVideoWnd};
const uintptr_t locator_UIEntryQueueHelpWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEntryQueueHelpWnd, (uintptr_t)hierarchy_UIEntryQueueHelpWnd};
const uintptr_t hierarchy_UIEntryQueueHelpWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIEntryQueueHelpWnd};
const uintptr_t bases_UIEntryQueueHelpWnd[] = {(uintptr_t)base_UIEntryQueueHelpWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEntryQueueHelpWnd[] = {(uintptr_t)&type_UIEntryQueueHelpWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEntryQueueHelpWnd};
const uintptr_t locator_UIEntryQueueListBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEntryQueueListBox, (uintptr_t)hierarchy_UIEntryQueueListBox};
const uintptr_t hierarchy_UIEntryQueueListBox[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIEntryQueueListBox};
const uintptr_t bases_UIEntryQueueListBox[] = {(uintptr_t)base_UIEntryQueueListBox, (uintptr_t)base_UIListBox, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEntryQueueListBox[] = {(uintptr_t)&type_UIEntryQueueListBox, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEntryQueueListBox};
const uintptr_t locator_UIEntryQueueRequestWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEntryQueueRequestWnd, (uintptr_t)hierarchy_UIEntryQueueRequestWnd};
const uintptr_t hierarchy_UIEntryQueueRequestWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIEntryQueueRequestWnd};
const uintptr_t bases_UIEntryQueueRequestWnd[] = {(uintptr_t)base_UIEntryQueueRequestWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEntryQueueRequestWnd[] = {(uintptr_t)&type_UIEntryQueueRequestWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEntryQueueRequestWnd};
const uintptr_t locator_UIEntryQueueStandByWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEntryQueueStandByWnd, (uintptr_t)hierarchy_UIEntryQueueStandByWnd};
const uintptr_t hierarchy_UIEntryQueueStandByWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIEntryQueueStandByWnd};
const uintptr_t bases_UIEntryQueueStandByWnd[] = {(uintptr_t)base_UIEntryQueueStandByWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEntryQueueStandByWnd[] = {(uintptr_t)&type_UIEntryQueueStandByWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEntryQueueStandByWnd};
const uintptr_t locator_UIEntryQueueWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)0, (uintptr_t)&type_UIEntryQueueWnd, (uintptr_t)hierarchy_UIEntryQueueWnd};
const uintptr_t hierarchy_UIEntryQueueWnd[] = {(uintptr_t)0, (uintptr_t)0, (uintptr_t)4, (uintptr_t)bases_UIEntryQueueWnd};
const uintptr_t bases_UIEntryQueueWnd[] = {(uintptr_t)base_UIEntryQueueWnd, (uintptr_t)base_UIFrameWnd, (uintptr_t)base_UIWindow, (uintptr_t)base_UIRPData};
const uintptr_t base_UIEntryQueueWnd[] = {(uintptr_t)&type_UIEntryQueueWnd, (uintptr_t)3, (uintptr_t)0, (uintptr_t)-1, (uintptr_t)0, (uintptr_t)64, (uintptr_t)hierarchy_UIEntryQueueWnd};

const struct ReferenceVtable reference_vtables[611] = {
    {locator_C3DCooRendering, {locator_C3DCooRendering}},
    {locator_C3dAnimModelRes, {locator_C3dAnimModelRes}},
    {locator_C3dAttr, {locator_C3dAttr}},
    {locator_C3dGrannyGameActor, {locator_C3dGrannyGameActor}},
    {locator_C3dGrannyModelRes, {locator_C3dGrannyModelRes}},
    {locator_C3dModelRes, {locator_C3dModelRes}},
    {locator_C3dModelResV1, {locator_C3dModelResV1}},
    {locator_C3dModelResV2, {locator_C3dModelResV2}},
    {locator_C3dWorldRes, {locator_C3dWorldRes}},
    {locator_CAMachineEffect, {locator_CAMachineEffect}},
    {locator_CAbleToMakeEffect, {locator_CAbleToMakeEffect}},
    {locator_CAcidifiedThrow, {locator_CAcidifiedThrow}},
    {locator_CActRes, {locator_CActRes}},
    {locator_CAnimatedEmitterEffect, {locator_CAnimatedEmitterEffect}},
    {locator_CArrowEffect, {locator_CArrowEffect}},
    {locator_CAstralStrikeEffect, {locator_CAstralStrikeEffect}},
    {locator_CAxeBoomerang, {locator_CAxeBoomerang}},
    {locator_CBash, {locator_CBash}},
    {locator_CBindTrapEffect, {locator_CBindTrapEffect}},
    {locator_CBitmapRes, {locator_CBitmapRes}},
    {locator_CBlastForge, {locator_CBlastForge}},
    {locator_CBlowEffect, {locator_CBlowEffect}},
    {locator_CBlurPC, {locator_CBlurPC}},
    {locator_CBmpObjWnd, {locator_CBmpObjWnd}},
    {locator_CCaneOfEvilEyeEffect, {locator_CCaneOfEvilEyeEffect}},
    {locator_CCartCannon, {locator_CCartCannon}},
    {locator_CCellRangeEffect, {locator_CCellRangeEffect}},
    {locator_CChangeSizeEffect, {locator_CChangeSizeEffect}},
    {locator_CCharacterNpc, {locator_CCharacterNpc}},
    {locator_CChargingPierceEffect, {locator_CChargingPierceEffect}},
    {locator_CCherryBlossomEffect, {locator_CCherryBlossomEffect}},
    {locator_CChoicePackageItemBoxWnd, {locator_CChoicePackageItemBoxWnd}},
    {locator_CClimaxEffect, {locator_CClimaxEffect}},
    {locator_CCloud, {locator_CCloud}},
    {locator_CCloudKillEffect, {locator_CCloudKillEffect}},
    {locator_CCommonFloorEffect, {locator_CCommonFloorEffect}},
    {locator_CCompetentia, {locator_CCompetentia}},
    {locator_CCorpse, {locator_CCorpse}},
    {locator_CCresciveBolt, {locator_CCresciveBolt}},
    {locator_CCrimsonArrowEffect, {locator_CCrimsonArrowEffect}},
    {locator_CCurseOfCubeEffect, {locator_CCurseOfCubeEffect}},
    {locator_CCylinderEffect, {locator_CCylinderEffect}},
    {locator_CDancingKnifeEffect, {locator_CDancingKnifeEffect}},
    {locator_CDummyEffectLauncher, {locator_CDummyEffectLauncher}},
    {locator_CEZ2STREffect, {locator_CEZ2STREffect}},
    {locator_CEZ2STREffectEx, {locator_CEZ2STREffectEx}},
    {locator_CEZeffectRes, {locator_CEZeffectRes}},
    {locator_CEffect, {locator_CEffect}},
    {locator_CEffectMaker, {locator_CEffectMaker}},
    {locator_CEffectPrim, {locator_CEffectPrim}},
    {locator_CElectricEffect, {locator_CElectricEffect}},
    {locator_CEmblemRes, {locator_CEmblemRes}},
    {locator_CEmitterEffect, {locator_CEmitterEffect}},
    {locator_CEnergyOrbEffect, {locator_CEnergyOrbEffect}},
    {locator_CEternalSlashAttackEffect, {locator_CEternalSlashAttackEffect}},
    {locator_CEternalSlashCountEffect, {locator_CEternalSlashCountEffect}},
    {locator_CEvilLandEffect, {locator_CEvilLandEffect}},
    {locator_CEvilsPawEffect, {locator_CEvilsPawEffect}},
    {locator_CFallenAngelEffect, {locator_CFallenAngelEffect}},
    {locator_CFloralFlareRoad, {locator_CFloralFlareRoad}},
    {locator_CFlowerCastingEffect, {locator_CFlowerCastingEffect}},
    {locator_CFortuneKiss, {locator_CFortuneKiss}},
    {locator_CFriggSongEffect, {locator_CFriggSongEffect}},
    {locator_CFromTheAbyssEffect, {locator_CFromTheAbyssEffect}},
    {locator_CFullScreenEffect, {locator_CFullScreenEffect}},
    {locator_CFuumaShouakuEffect, {locator_CFuumaShouakuEffect}},
    {locator_CGaleStorm, {locator_CGaleStorm}},
    {locator_CGameActor, {locator_CGameActor}},
    {locator_CGifRes, {locator_CGifRes}},
    {locator_CGlassWall, {locator_CGlassWall}},
    {locator_CGndRes, {locator_CGndRes}},
    {locator_CGndRiser, {locator_CGndRiser}},
    {locator_CGraffiSkill, {locator_CGraffiSkill}},
    {locator_CGrannyPc, {locator_CGrannyPc}},
    {locator_CGravityControlEffect, {locator_CGravityControlEffect}},
    {locator_CGroundScope, {locator_CGroundScope}},
    {locator_CHalluNPC, {locator_CHalluNPC}},
    {locator_CHalluPC, {locator_CHalluPC}},
    {locator_CHalluPC2, {locator_CHalluPC2}},
    {locator_CHalluPlant, {locator_CHalluPlant}},
    {locator_CHawkVumerangEffect, {locator_CHawkVumerangEffect}},
    {locator_CHealEffect, {locator_CHealEffect}},
    {locator_CHealSp, {locator_CHealSp}},
    {locator_CHitLineEffect, {locator_CHitLineEffect}},
    {locator_CIcon, {locator_CIcon}},
    {locator_CImfRes, {locator_CImfRes}},
    {locator_CItem, {locator_CItem}},
    {locator_CItemDropEffect, {locator_CItemDropEffect}},
    {locator_CJupitelThunderHit, {locator_CJupitelThunderHit}},
    {locator_CJupitelThunderStormEffect, {locator_CJupitelThunderStormEffect}},
    {locator_CLevel150Effect, {locator_CLevel150Effect}},
    {locator_CLevel150SubEffect, {locator_CLevel150SubEffect}},
    {locator_CLevel99Effect, {locator_CLevel99Effect}},
    {locator_CLevel99Orb1Effect, {locator_CLevel99Orb1Effect}},
    {locator_CLevel99Orb2Effect, {locator_CLevel99Orb2Effect}},
    {locator_CLightSphereEffect, {locator_CLightSphereEffect}},
    {locator_CMagicFloorEffect, {locator_CMagicFloorEffect}},
    {locator_CMagnumBreak, {locator_CMagnumBreak}},
    {locator_CMandraAttackEffect, {locator_CMandraAttackEffect}},
    {locator_CMapChainEffect, {locator_CMapChainEffect}},
    {locator_CMapMagicZoneEffect, {locator_CMapMagicZoneEffect}},
    {locator_CMappillarEffect, {locator_CMappillarEffect}},
    {locator_CMasterChaser, {locator_CMasterChaser}},
    {locator_CMergeItemWnd, {locator_CMergeItemWnd}},
    {locator_CMeteorEffect, {locator_CMeteorEffect}},
    {locator_CMousePointer, {locator_CMousePointer}},
    {locator_CMsgEffect, {locator_CMsgEffect}},
    {locator_CNewCloudKillEffect, {locator_CNewCloudKillEffect}},
    {locator_CNpc, {locator_CNpc}},
    {locator_CNumEffect, {locator_CNumEffect}},
    {locator_CNyanggrassEffect, {locator_CNyanggrassEffect}},
    {locator_CPaletteRes, {locator_CPaletteRes}},
    {locator_CPc, {locator_CPc}},
    {locator_CPetitioEffect, {locator_CPetitioEffect}},
    {locator_CPlayer, {locator_CPlayer}},
    {locator_CPneumaticusProcellaEffect, {locator_CPneumaticusProcellaEffect}},
    {locator_CPokJukEffect, {locator_CPokJukEffect}},
    {locator_CPowderingEffect, {locator_CPowderingEffect}},
    {locator_CRagEffect, {locator_CRagEffect}},
    {locator_CRainbowEffect, {locator_CRainbowEffect}},
    {locator_CRandomHitEffect, {locator_CRandomHitEffect}},
    {locator_CRandomOptEffect, {locator_CRandomOptEffect}},
    {locator_CRectUpEffect, {locator_CRectUpEffect}},
    {locator_CRedLineEffect, {locator_CRedLineEffect}},
    {locator_CRenderObject, {locator_CRenderObject}},
    {locator_CRes, {locator_CRes}},
    {locator_CRichManKim, {locator_CRichManKim}},
    {locator_CRichsCoinEffect, {locator_CRichsCoinEffect}},
    {locator_CRollingCutterRagEffect, {locator_CRollingCutterRagEffect}},
    {locator_CRootTwistEffect, {locator_CRootTwistEffect}},
    {locator_CRotateBgEffect, {locator_CRotateBgEffect}},
    {locator_CRotateItemEffect, {locator_CRotateItemEffect}},
    {locator_CRotateLineEffect, {locator_CRotateLineEffect}},
    {locator_CSaintCastingEffect, {locator_CSaintCastingEffect}},
    {locator_CSavageImpactEffect, {locator_CSavageImpactEffect}},
    {locator_CScatterEffect, {locator_CScatterEffect}},
    {locator_CServantWeaponEffect, {locator_CServantWeaponEffect}},
    {locator_CSevereRainStorm, {locator_CSevereRainStorm}},
    {locator_CShadowExceed, {locator_CShadowExceed}},
    {locator_CShowEffect, {locator_CShowEffect}},
    {locator_CSignBoardWnd, {locator_CSignBoardWnd}},
    {locator_CSkill, {locator_CSkill}},
    {locator_CSmokeEffect, {locator_CSmokeEffect}},
    {locator_CSoulCollectEffect, {locator_CSoulCollectEffect}},
    {locator_CSoulUnityEffect, {locator_CSoulUnityEffect}},
    {locator_CSoundMaker, {locator_CSoundMaker}},
    {locator_CSphereWind2Effect, {locator_CSphereWind2Effect}},
    {locator_CSphereWindEffect, {locator_CSphereWindEffect}},
    {locator_CSprRes, {locator_CSprRes}},
    {locator_CSpriteEffect, {locator_CSpriteEffect}},
    {locator_CSpriteMableEffect, {locator_CSpriteMableEffect}},
    {locator_CSquareRangeEffect, {locator_CSquareRangeEffect}},
    {locator_CStarEffect, {locator_CStarEffect}},
    {locator_CStarEmperorEffect, {locator_CStarEmperorEffect}},
    {locator_CSteal, {locator_CSteal}},
    {locator_CStemSpearEffect, {locator_CStemSpearEffect}},
    {locator_CStormCannonEffect, {locator_CStormCannonEffect}},
    {locator_CStormGustEffect, {locator_CStormGustEffect}},
    {locator_CStormKickEffect, {locator_CStormKickEffect}},
    {locator_CSunStanceEffect, {locator_CSunStanceEffect}},
    {locator_CSvgSpiritEffect, {locator_CSvgSpiritEffect}},
    {locator_CTalismanMarkEffect, {locator_CTalismanMarkEffect}},
    {locator_CTetraBallEffect, {locator_CTetraBallEffect}},
    {locator_CThrowItemEffect, {locator_CThrowItemEffect}},
    {locator_CTriangleShotArrow, {locator_CTriangleShotArrow}},
    {locator_CTunaPartyEffect, {locator_CTunaPartyEffect}},
    {locator_CUICollectionSystemWnd, {locator_CUICollectionSystemWnd}},
    {locator_CViolentQuakeEffect, {locator_CViolentQuakeEffect}},
    {locator_CVolcanoEffect, {locator_CVolcanoEffect}},
    {locator_CWarningPlaneEffect, {locator_CWarningPlaneEffect}},
    {locator_CWave, {locator_CWave}},
    {locator_CWindEffect, {locator_CWindEffect}},
    {locator_TableBox, {locator_TableBox}},
    {locator_UI2ndCostumeCheckBox, {locator_UI2ndCostumeCheckBox}},
    {locator_UI2ndCostumeWnd, {locator_UI2ndCostumeWnd}},
    {locator_UIAccBitmapButton, {locator_UIAccBitmapButton}},
    {locator_UIAccessoryCusWnd, {locator_UIAccessoryCusWnd}},
    {locator_UIAccessoryShowCase, {locator_UIAccessoryShowCase}},
    {locator_UIAccountLimitedToolWnd, {locator_UIAccountLimitedToolWnd}},
    {locator_UIAchBalloonText, {locator_UIAchBalloonText}},
    {locator_UIAchListWnd, {locator_UIAchListWnd}},
    {locator_UIAchScrollBar, {locator_UIAchScrollBar}},
    {locator_UIAchSummaryWnd, {locator_UIAchSummaryWnd}},
    {locator_UIAchTV, {locator_UIAchTV}},
    {locator_UIAchTab, {locator_UIAchTab}},
    {locator_UIAchTracingWnd, {locator_UIAchTracingWnd}},
    {locator_UIAchUpperWnd, {locator_UIAchUpperWnd}},
    {locator_UIAchViewWnd, {locator_UIAchViewWnd}},
    {locator_UIAchievementWnd, {locator_UIAchievementWnd}},
    {locator_UIAdvenPartyBoardWnd, {locator_UIAdvenPartyBoardWnd}},
    {locator_UIAgitMessageBox, {locator_UIAgitMessageBox}},
    {locator_UIAllyGuildAcceptWnd, {locator_UIAllyGuildAcceptWnd}},
    {locator_UIApplyForPartyWnd, {locator_UIApplyForPartyWnd}},
    {locator_UIAutoMessageWnd, {locator_UIAutoMessageWnd}},
    {locator_UIBabyAcceptWnd, {locator_UIBabyAcceptWnd}},
    {locator_UIBabyAcceptWnd2, {locator_UIBabyAcceptWnd2}},
    {locator_UIBalloonText, {locator_UIBalloonText}},
    {locator_UIBank_NewWnd, {locator_UIBank_NewWnd}},
    {locator_UIBarGraph, {locator_UIBarGraph}},
    {locator_UIBarGraph2, {locator_UIBarGraph2}},
    {locator_UIBarGraph3, {locator_UIBarGraph3}},
    {locator_UIBarGraph4, {locator_UIBarGraph4}},
    {locator_UIBarGraphHomun, {locator_UIBarGraphHomun}},
    {locator_UIBarGraphPlayer, {locator_UIBarGraphPlayer}},
    {locator_UIBarGraphPlayerAP, {locator_UIBarGraphPlayerAP}},
    {locator_UIBarterItemPurchaseWnd, {locator_UIBarterItemPurchaseWnd}},
    {locator_UIBartermarketWnd, {locator_UIBartermarketWnd}},
    {locator_UIBasicButton, {locator_UIBasicButton}},
    {locator_UIBasicInfoWnd, {locator_UIBasicInfoWnd}},
    {locator_UIBasicItemCtrl, {locator_UIBasicItemCtrl}},
    {locator_UIBasicSkillCtrl, {locator_UIBasicSkillCtrl}},
    {locator_UIBasicStylingWnd, {locator_UIBasicStylingWnd}},
    {locator_UIBattleMsgOptionWnd, {locator_UIBattleMsgOptionWnd}},
    {locator_UIBitmapButton, {locator_UIBitmapButton}},
    {locator_UIBitmapButton2, {locator_UIBitmapButton2}},
    {locator_UIBitmapEditCtrl, {locator_UIBitmapEditCtrl}},
    {locator_UIBitmapTab, {locator_UIBitmapTab}},
    {locator_UIBmpRadioBtn, {locator_UIBmpRadioBtn}},
    {locator_UIBmpTextButton, {locator_UIBmpTextButton}},
    {locator_UIBmpTextButton2, {locator_UIBmpTextButton2}},
    {locator_UIBookWnd, {locator_UIBookWnd}},
    {locator_UIButton, {locator_UIButton}},
    {locator_UIButtonsOfBasicWnd, {locator_UIButtonsOfBasicWnd}},
    {locator_UICandidateWnd, {locator_UICandidateWnd}},
    {locator_UICaptchaImage, {locator_UICaptchaImage}},
    {locator_UICaptchaPreviewWnd, {locator_UICaptchaPreviewWnd}},
    {locator_UICaptchaRegisterWnd, {locator_UICaptchaRegisterWnd}},
    {locator_UICardItemIllustWnd, {locator_UICardItemIllustWnd}},
    {locator_UICashShopWnd, {locator_UICashShopWnd}},
    {locator_UICashShopWnd_Banner, {locator_UICashShopWnd_Banner}},
    {locator_UICashShopWnd_Cart, {locator_UICashShopWnd_Cart}},
    {locator_UICashShopWnd_CartItem, {locator_UICashShopWnd_CartItem}},
    {locator_UICashShopWnd_Item, {locator_UICashShopWnd_Item}},
    {locator_UIChangeNameWnd, {locator_UIChangeNameWnd}},
    {locator_UICharInfoBalloonText, {locator_UICharInfoBalloonText}},
    {locator_UIChatEditCtrl, {locator_UIChatEditCtrl}},
    {locator_UIChatHisBox, {locator_UIChatHisBox}},
    {locator_UIChatHisBox2, {locator_UIChatHisBox2}},
    {locator_UIChatHisBox3, {locator_UIChatHisBox3}},
    {locator_UIChatRoomChangeWnd, {locator_UIChatRoomChangeWnd}},
    {locator_UIChatRoomMakeWnd, {locator_UIChatRoomMakeWnd}},
    {locator_UIChatRoomTitle, {locator_UIChatRoomTitle}},
    {locator_UIChatRoomWnd, {locator_UIChatRoomWnd}},
    {locator_UICheckAttendanceWnd, {locator_UICheckAttendanceWnd}},
    {locator_UICheckAttendanceWnd_Banner, {locator_UICheckAttendanceWnd_Banner}},
    {locator_UICheckBox, {locator_UICheckBox}},
    {locator_UIChooseSellBuyWnd, {locator_UIChooseSellBuyWnd}},
    {locator_UIChooseWarpWnd, {locator_UIChooseWarpWnd}},
    {locator_UIChooseWnd, {locator_UIChooseWnd}},
    {locator_UIClanInfoManageWnd, {locator_UIClanInfoManageWnd}},
    {locator_UIClanWnd, {locator_UIClanWnd}},
    {locator_UICntWnd, {locator_UICntWnd}},
    {locator_UIColorButton, {locator_UIColorButton}},
    {locator_UIColorTransBalloonText, {locator_UIColorTransBalloonText}},
    {locator_UIColoringTextViewer, {locator_UIColoringTextViewer}},
    {locator_UICombinedCardItemCollectionWnd, {locator_UICombinedCardItemCollectionWnd}},
    {locator_UICombinedCardItemCollection_ComparisonWnd, {locator_UICombinedCardItemCollection_ComparisonWnd}},
    {locator_UIComboBox, {locator_UIComboBox}},
    {locator_UIComboBoxDef2Wnd, {locator_UIComboBoxDef2Wnd}},
    {locator_UIComboBoxDefWnd, {locator_UIComboBoxDefWnd}},
    {locator_UIComboBoxEx, {locator_UIComboBoxEx}},
    {locator_UIComboBoxEx2, {locator_UIComboBoxEx2}},
    {locator_UIComboBoxID, {locator_UIComboBoxID}},
    {locator_UIComboBoxIDWnd, {locator_UIComboBoxIDWnd}},
    {locator_UIComboBoxV2Wnd, {locator_UIComboBoxV2Wnd}},
    {locator_UIComboBoxWnd, {locator_UIComboBoxWnd}},
    {locator_UIComboBox_BmpWnd, {locator_UIComboBox_BmpWnd}},
    {locator_UICommandListWnd, {locator_UICommandListWnd}},
    {locator_UICompositionWnd, {locator_UICompositionWnd}},
    {locator_UIConnectSnsWnd, {locator_UIConnectSnsWnd}},
    {locator_UIConnectTwitterWnd, {locator_UIConnectTwitterWnd}},
    {locator_UIControlRectWnd, {locator_UIControlRectWnd}},
    {locator_UICoupleAcceptWnd, {locator_UICoupleAcceptWnd}},
    {locator_UICreateGuildWnd, {locator_UICreateGuildWnd}},
    {locator_UICustomGageBar, {locator_UICustomGageBar}},
    {locator_UIDescriptRace, {locator_UIDescriptRace}},
    {locator_UIDisconnectedServerMsgWnd, {locator_UIDisconnectedServerMsgWnd}},
    {locator_UIDisplayCharacter, {locator_UIDisplayCharacter}},
    {locator_UIDragMiniPartyWnd, {locator_UIDragMiniPartyWnd}},
    {locator_UIEditCtrl, {locator_UIEditCtrl}},
    {locator_UIEditCtrl2, {locator_UIEditCtrl2}},
    {locator_UIEditCtrl3, {locator_UIEditCtrl3}},
    {locator_UIEditCtrl4, {locator_UIEditCtrl4}},
    {locator_UIEditCtrl5, {locator_UIEditCtrl5}},
    {locator_UIEditCtrlVarId, {locator_UIEditCtrlVarId}},
    {locator_UIEditCtrlWheel, {locator_UIEditCtrlWheel}},
    {locator_UIEditCtrl_Currency, {locator_UIEditCtrl_Currency}},
    {locator_UIEditCtrl_Custom, {locator_UIEditCtrl_Custom}},
    {locator_UIEditCtrl_OldList, {locator_UIEditCtrl_OldList}},
    {locator_UIEditctrlWnd, {locator_UIEditctrlWnd}},
    {locator_UIEmailAddressWnd, {locator_UIEmailAddressWnd}},
    {locator_UIEmblemWnd, {locator_UIEmblemWnd}},
    {locator_UIEmotionListWnd, {locator_UIEmotionListWnd}},
    {locator_UIEmotionWnd, {locator_UIEmotionWnd}},
    {locator_UIEnchantWnd, {locator_UIEnchantWnd}},
    {locator_UIEnchant_Button, {locator_UIEnchant_Button}},
    {locator_UIEnchant_Group, {locator_UIEnchant_Group}},
    {locator_UIEnchant_Info, {locator_UIEnchant_Info}},
    {locator_UIEnchant_Item, {locator_UIEnchant_Item}},
    {locator_UIEnchant_List, {locator_UIEnchant_List}},
    {locator_UIEnchant_ScrollBar, {locator_UIEnchant_ScrollBar}},
    {locator_UIEnchant_Tab, {locator_UIEnchant_Tab}},
    {locator_UIEnchant_TargetList, {locator_UIEnchant_TargetList}},
    {locator_UIEntryQueueHelpWnd, {locator_UIEntryQueueHelpWnd}},
    {locator_UIEntryQueueListBox, {locator_UIEntryQueueListBox}},
    {locator_UIEntryQueueRequestWnd, {locator_UIEntryQueueRequestWnd}},
    {locator_UIEntryQueueStandByWnd, {locator_UIEntryQueueStandByWnd}},
    {locator_UIEntryQueueWnd, {locator_UIEntryQueueWnd}},
    {locator_UIEquipWnd, {locator_UIEquipWnd}},
    {locator_UIEscOptionWnd, {locator_UIEscOptionWnd}},
    {locator_UIExchangeAcceptWnd, {locator_UIExchangeAcceptWnd}},
    {locator_UIExchangeWnd, {locator_UIExchangeWnd}},
    {locator_UIExpandedBarterItemPurchaseWnd, {locator_UIExpandedBarterItemPurchaseWnd}},
    {locator_UIExpandedBarterPurchaseButton, {locator_UIExpandedBarterPurchaseButton}},
    {locator_UIExpandedBartermarketWnd, {locator_UIExpandedBartermarketWnd}},
    {locator_UIFrameWnd, {locator_UIFrameWnd}},
    {locator_UIFriendOptionWnd, {locator_UIFriendOptionWnd}},
    {locator_UIGage, {locator_UIGage}},
    {locator_UIGradeEnchantWnd, {locator_UIGradeEnchantWnd}},
    {locator_UIGradeEnchant_BtnEnhancedDown, {locator_UIGradeEnchant_BtnEnhancedDown}},
    {locator_UIGradeEnchant_BtnEnhancedUp, {locator_UIGradeEnchant_BtnEnhancedUp}},
    {locator_UIGradeEnchant_BtnStart, {locator_UIGradeEnchant_BtnStart}},
    {locator_UIGraffiStrboxWnd, {locator_UIGraffiStrboxWnd}},
    {locator_UIGraphicSettingWnd, {locator_UIGraphicSettingWnd}},
    {locator_UIGuildBanReasonDescWnd, {locator_UIGuildBanReasonDescWnd}},
    {locator_UIGuildBanishedMemberWnd, {locator_UIGuildBanishedMemberWnd}},
    {locator_UIGuildHelperWnd, {locator_UIGuildHelperWnd}},
    {locator_UIGuildInfoManageWnd, {locator_UIGuildInfoManageWnd}},
    {locator_UIGuildLeaveReasonDescWnd, {locator_UIGuildLeaveReasonDescWnd}},
    {locator_UIGuildMemberManageWnd, {locator_UIGuildMemberManageWnd}},
    {locator_UIGuildNoticeWnd, {locator_UIGuildNoticeWnd}},
    {locator_UIGuildPositionManageWnd, {locator_UIGuildPositionManageWnd}},
    {locator_UIGuildSkillWnd, {locator_UIGuildSkillWnd}},
    {locator_UIGuildTipWnd, {locator_UIGuildTipWnd}},
    {locator_UIGuildTotalInfoWnd, {locator_UIGuildTotalInfoWnd}},
    {locator_UIGuildWnd, {locator_UIGuildWnd}},
    {locator_UIGuild_Storage_Log, {locator_UIGuild_Storage_Log}},
    {locator_UIHairBitmap, {locator_UIHairBitmap}},
    {locator_UIHairStyleButton, {locator_UIHairStyleButton}},
    {locator_UIHeadsprButton, {locator_UIHeadsprButton}},
    {locator_UIHomunInfoWnd, {locator_UIHomunInfoWnd}},
    {locator_UIHotKeyWnd, {locator_UIHotKeyWnd}},
    {locator_UIHotkeyGuideWnd, {locator_UIHotkeyGuideWnd}},
    {locator_UIINT64BarGraph, {locator_UIINT64BarGraph}},
    {locator_UIIllustWnd, {locator_UIIllustWnd}},
    {locator_UIImgMenu, {locator_UIImgMenu}},
    {locator_UIIngredientInfoWnd, {locator_UIIngredientInfoWnd}},
    {locator_UIItemBaseWnd, {locator_UIItemBaseWnd}},
    {locator_UIItemCollectionWnd, {locator_UIItemCollectionWnd}},
    {locator_UIItemCollection_ComparisonWnd, {locator_UIItemCollection_ComparisonWnd}},
    {locator_UIItemCompositionWnd, {locator_UIItemCompositionWnd}},
    {locator_UIItemDropCntWnd, {locator_UIItemDropCntWnd}},
    {locator_UIItemIdentifyWnd, {locator_UIItemIdentifyWnd}},
    {locator_UIItemMoveInfo, {locator_UIItemMoveInfo}},
    {locator_UIItemParamChangeDisplayWnd, {locator_UIItemParamChangeDisplayWnd}},
    {locator_UIItemPurchaseWnd, {locator_UIItemPurchaseWnd}},
    {locator_UIItemReformWnd, {locator_UIItemReformWnd}},
    {locator_UIItemReformWnd_DetailBtn, {locator_UIItemReformWnd_DetailBtn}},
    {locator_UIItemReformWnd_DetailMsg, {locator_UIItemReformWnd_DetailMsg}},
    {locator_UIItemReformWnd_Item, {locator_UIItemReformWnd_Item}},
    {locator_UIItemReformWnd_List, {locator_UIItemReformWnd_List}},
    {locator_UIItemRepairWnd, {locator_UIItemRepairWnd}},
    {locator_UIItemSellWnd, {locator_UIItemSellWnd}},
    {locator_UIItemSellWnd2, {locator_UIItemSellWnd2}},
    {locator_UIItemShopWnd, {locator_UIItemShopWnd}},
    {locator_UIItemShopWnd2, {locator_UIItemShopWnd2}},
    {locator_UIItemStoreComboBox, {locator_UIItemStoreComboBox}},
    {locator_UIItemStoreComboBoxWnd, {locator_UIItemStoreComboBoxWnd}},
    {locator_UIItemStoreFindWnd, {locator_UIItemStoreFindWnd}},
    {locator_UIItemStoreSubWnd, {locator_UIItemStoreSubWnd}},
    {locator_UIItemStoreWnd, {locator_UIItemStoreWnd}},
    {locator_UIItemTagButton, {locator_UIItemTagButton}},
    {locator_UIItemTagCtrl, {locator_UIItemTagCtrl}},
    {locator_UIItemTagOnChat, {locator_UIItemTagOnChat}},
    {locator_UIItemTagOnMail, {locator_UIItemTagOnMail}},
    {locator_UIItemTagOnMegaPhone, {locator_UIItemTagOnMegaPhone}},
    {locator_UIItemWnd, {locator_UIItemWnd}},
    {locator_UIJobIconButton, {locator_UIJobIconButton}},
    {locator_UIJobListWnd, {locator_UIJobListWnd}},
    {locator_UIJobSlotButton, {locator_UIJobSlotButton}},
    {locator_UIJoinFriendAcceptWnd, {locator_UIJoinFriendAcceptWnd}},
    {locator_UIJoinGuildAcceptWnd, {locator_UIJoinGuildAcceptWnd}},
    {locator_UIJoinPartyAcceptWnd, {locator_UIJoinPartyAcceptWnd}},
    {locator_UIKeyStrokeWnd, {locator_UIKeyStrokeWnd}},
    {locator_UILapineBoxWnd, {locator_UILapineBoxWnd}},
    {locator_UILapineBox_IngredientListWnd, {locator_UILapineBox_IngredientListWnd}},
    {locator_UILapineScrollBar, {locator_UILapineScrollBar}},
    {locator_UILapineUpgradeBoxWnd, {locator_UILapineUpgradeBoxWnd}},
    {locator_UILapineUpgradeBox_IngredientInfoWnd, {locator_UILapineUpgradeBox_IngredientInfoWnd}},
    {locator_UILapineUpgradeBox_IngredientListWnd, {locator_UILapineUpgradeBox_IngredientListWnd}},
    {locator_UIListBox, {locator_UIListBox}},
    {locator_UIListBox2, {locator_UIListBox2}},
    {locator_UIListBoxAgit, {locator_UIListBoxAgit}},
    {locator_UIListBoxAgitMenu, {locator_UIListBoxAgitMenu}},
    {locator_UIListBoxEx, {locator_UIListBoxEx}},
    {locator_UIListBoxID, {locator_UIListBoxID}},
    {locator_UIListBoxNavi, {locator_UIListBoxNavi}},
    {locator_UIListBoxTitle, {locator_UIListBoxTitle}},
    {locator_UIListBox_NaviSearch, {locator_UIListBox_NaviSearch}},
    {locator_UILoadingWnd, {locator_UILoadingWnd}},
    {locator_UILoginOTPWnd, {locator_UILoginOTPWnd}},
    {locator_UILoginWnd, {locator_UILoginWnd}},
    {locator_UIMacroDetectorWnd, {locator_UIMacroDetectorWnd}},
    {locator_UIMacroReporterWnd, {locator_UIMacroReporterWnd}},
    {locator_UIMacroUserInfoWnd, {locator_UIMacroUserInfoWnd}},
    {locator_UIMakeCharWnd, {locator_UIMakeCharWnd}},
    {locator_UIMakeTargetListWnd, {locator_UIMakeTargetListWnd}},
    {locator_UIMakeTargetProcessWnd, {locator_UIMakeTargetProcessWnd}},
    {locator_UIMakeTargetResultWnd, {locator_UIMakeTargetResultWnd}},
    {locator_UIMakingArrowListWnd, {locator_UIMakingArrowListWnd}},
    {locator_UIMemberListBox, {locator_UIMemberListBox}},
    {locator_UIMemorialDunWnd, {locator_UIMemorialDunWnd}},
    {locator_UIMenuIcon, {locator_UIMenuIcon}},
    {locator_UIMenuIconWnd, {locator_UIMenuIconWnd}},
    {locator_UIMenuWnd, {locator_UIMenuWnd}},
    {locator_UIMerInfoWnd, {locator_UIMerInfoWnd}},
    {locator_UIMerchantItemLogWnd, {locator_UIMerchantItemLogWnd}},
    {locator_UIMerchantItemMyShopWnd, {locator_UIMerchantItemMyShopWnd}},
    {locator_UIMerchantItemPurchaseMSGWnd, {locator_UIMerchantItemPurchaseMSGWnd}},
    {locator_UIMerchantItemPurchaseWnd, {locator_UIMerchantItemPurchaseWnd}},
    {locator_UIMerchantItemShopWnd, {locator_UIMerchantItemShopWnd}},
    {locator_UIMerchantItemWnd, {locator_UIMerchantItemWnd}},
    {locator_UIMerchantMirrorItemWnd, {locator_UIMerchantMirrorItemWnd}},
    {locator_UIMerchantShopMakeWnd, {locator_UIMerchantShopMakeWnd}},
    {locator_UIMerchantShopTitle, {locator_UIMerchantShopTitle}},
    {locator_UIMessageBox, {locator_UIMessageBox}},
    {locator_UIMessageBoxAutoreturn, {locator_UIMessageBoxAutoreturn}},
    {locator_UIMessengerGroupWnd, {locator_UIMessengerGroupWnd}},
    {locator_UIMileageWnd, {locator_UIMileageWnd}},
    {locator_UIMiniBtnWnd, {locator_UIMiniBtnWnd}},
    {locator_UIMiniMapWnd, {locator_UIMiniMapWnd}},
    {locator_UIMiniPartyInfoWnd, {locator_UIMiniPartyInfoWnd}},
    {locator_UIMiniPartyWnd, {locator_UIMiniPartyWnd}},
    {locator_UIMinimapZoomWnd, {locator_UIMinimapZoomWnd}},
    {locator_UIMixAcceptWnd, {locator_UIMixAcceptWnd}},
    {locator_UIMonsterDisplayWnd, {locator_UIMonsterDisplayWnd}},
    {locator_UIMonsterGage, {locator_UIMonsterGage}},
    {locator_UIMonsterInfoWnd, {locator_UIMonsterInfoWnd}},
    {locator_UIMyStyleWnd, {locator_UIMyStyleWnd}},
    {locator_UINameBalloonText, {locator_UINameBalloonText}},
    {locator_UINaviLinkButton, {locator_UINaviLinkButton}},
    {locator_UINavigationHelpWnd, {locator_UINavigationHelpWnd}},
    {locator_UINavigationRuideWnd, {locator_UINavigationRuideWnd}},
    {locator_UINavigationV4Wnd, {locator_UINavigationV4Wnd}},
    {locator_UINavigationWnd, {locator_UINavigationWnd}},
    {locator_UINavigationroadiconWnd, {locator_UINavigationroadiconWnd}},
    {locator_UINewChatWnd, {locator_UINewChatWnd}},
    {locator_UINewComboBoxCtrl, {locator_UINewComboBoxCtrl}},
    {locator_UINewMakeCharWnd, {locator_UINewMakeCharWnd}},
    {locator_UINewSelectCharWnd, {locator_UINewSelectCharWnd}},
    {locator_UINewSkillListWnd, {locator_UINewSkillListWnd}},
    {locator_UINoneVisibleButton, {locator_UINoneVisibleButton}},
    {locator_UINormalMB, {locator_UINormalMB}},
    {locator_UINoteWnd, {locator_UINoteWnd}},
    {locator_UINoteWnd_Scroll, {locator_UINoteWnd_Scroll}},
    {locator_UINoticeConfirmWnd, {locator_UINoticeConfirmWnd}},
    {locator_UINoticeMessageBox_LockSellItem, {locator_UINoticeMessageBox_LockSellItem}},
    {locator_UINoticeWnd, {locator_UINoticeWnd}},
    {locator_UINotifyItemObtainWnd, {locator_UINotifyItemObtainWnd}},
    {locator_UINotifyJobLevelUpWnd, {locator_UINotifyJobLevelUpWnd}},
    {locator_UINotifyLevelUpWnd, {locator_UINotifyLevelUpWnd}},
    {locator_UINotifyQuestWnd, {locator_UINotifyQuestWnd}},
    {locator_UINpcEditDialogWnd, {locator_UINpcEditDialogWnd}},
    {locator_UINpcSprWnd, {locator_UINpcSprWnd}},
    {locator_UINpcTextEditDialogWnd, {locator_UINpcTextEditDialogWnd}},
    {locator_UIOngoingQuestScrollWnd, {locator_UIOngoingQuestScrollWnd}},
    {locator_UIOpenMailBoxWnd, {locator_UIOpenMailBoxWnd}},
    {locator_UIOpenRoulletteWnd, {locator_UIOpenRoulletteWnd}},
    {locator_UIPackageBoxNameList, {locator_UIPackageBoxNameList}},
    {locator_UIParaItemPurchaseWnd, {locator_UIParaItemPurchaseWnd}},
    {locator_UIParaItemShopWnd, {locator_UIParaItemShopWnd}},
    {locator_UIParaResultWnd, {locator_UIParaResultWnd}},
    {locator_UIPartyBookingHelpWnd, {locator_UIPartyBookingHelpWnd}},
    {locator_UIPartyInfoWnd, {locator_UIPartyInfoWnd}},
    {locator_UIPartyInvitationToWnd, {locator_UIPartyInvitationToWnd}},
    {locator_UIPartySettingWnd, {locator_UIPartySettingWnd}},
    {locator_UIPasswordWnd, {locator_UIPasswordWnd}},
    {locator_UIPcGage, {locator_UIPcGage}},
    {locator_UIPetEggListWnd, {locator_UIPetEggListWnd}},
    {locator_UIPetEvolutionWnd, {locator_UIPetEvolutionWnd}},
    {locator_UIPetInfoBarGraph, {locator_UIPetInfoBarGraph}},
    {locator_UIPetInfoWnd, {locator_UIPetInfoWnd}},
    {locator_UIPetTamingDeceiveWnd, {locator_UIPetTamingDeceiveWnd}},
    {locator_UIPlayerGage, {locator_UIPlayerGage}},
    {locator_UIPreviewEquipWnd, {locator_UIPreviewEquipWnd}},
    {locator_UIPrivateAirplaneWnd, {locator_UIPrivateAirplaneWnd}},
    {locator_UIProhibitListWnd, {locator_UIProhibitListWnd}},
    {locator_UIPurchaseButton, {locator_UIPurchaseButton}},
    {locator_UIQuestDisplay, {locator_UIQuestDisplay}},
    {locator_UIQuikSlotWnd, {locator_UIQuikSlotWnd}},
    {locator_UIQuitWnd, {locator_UIQuitWnd}},
    {locator_UIRPImageWnd, {locator_UIRPImageWnd}},
    {locator_UIRPImageWnd2, {locator_UIRPImageWnd2}},
    {locator_UIRadioBtn, {locator_UIRadioBtn}},
    {locator_UIRebirthWnd, {locator_UIRebirthWnd}},
    {locator_UIRechargeGage, {locator_UIRechargeGage}},
    {locator_UIRecommededQuestScrollWnd, {locator_UIRecommededQuestScrollWnd}},
    {locator_UIRefiningWnd, {locator_UIRefiningWnd}},
    {locator_UIRefining_BtnAgain, {locator_UIRefining_BtnAgain}},
    {locator_UIRefining_BtnRefining, {locator_UIRefining_BtnRefining}},
    {locator_UIRefining_BtnReturn, {locator_UIRefining_BtnReturn}},
    {locator_UIRefining_Notice, {locator_UIRefining_Notice}},
    {locator_UIRegisterPartyWnd, {locator_UIRegisterPartyWnd}},
    {locator_UIRenderTargetWnd, {locator_UIRenderTargetWnd}},
    {locator_UIRenewDetailQuestInfoWnd, {locator_UIRenewDetailQuestInfoWnd}},
    {locator_UIRenewQuestScrollBar, {locator_UIRenewQuestScrollBar}},
    {locator_UIRenewQuestWnd, {locator_UIRenewQuestWnd}},
    {locator_UIReplayButton, {locator_UIReplayButton}},
    {locator_UIReplayControlWnd, {locator_UIReplayControlWnd}},
    {locator_UIReplayListBox, {locator_UIReplayListBox}},
    {locator_UIReplayRECControlWnd, {locator_UIReplayRECControlWnd}},
    {locator_UIReputeWnd, {locator_UIReputeWnd}},
    {locator_UIRequestJoinPartyWnd, {locator_UIRequestJoinPartyWnd}},
    {locator_UIResizer, {locator_UIResizer}},
    {locator_UIRestartWnd, {locator_UIRestartWnd}},
    {locator_UIRoMapWnd, {locator_UIRoMapWnd}},
    {locator_UIRodexComposeWnd, {locator_UIRodexComposeWnd}},
    {locator_UIRodexContentsWnd, {locator_UIRodexContentsWnd}},
    {locator_UIRodexEditCtrl3, {locator_UIRodexEditCtrl3}},
    {locator_UIRodexInboxWnd, {locator_UIRodexInboxWnd}},
    {locator_UIRodexScrollBar, {locator_UIRodexScrollBar}},
    {locator_UIRoulletteWnd, {locator_UIRoulletteWnd}},
    {locator_UIRoundBoxTooltipWnd, {locator_UIRoundBoxTooltipWnd}},
    {locator_UISayDialogWnd, {locator_UISayDialogWnd}},
    {locator_TagTextViewer_UIScroll, {locator_TagTextViewer_UIScroll}},
    {locator_UIScrollBar, {locator_UIScrollBar}},
    {locator_UIScrollBar2, {locator_UIScrollBar2}},
    {locator_UIScrollBar3, {locator_UIScrollBar3}},
    {locator_UIScrollBar4, {locator_UIScrollBar4}},
    {locator_UIScrollBar_Custom, {locator_UIScrollBar_Custom}},
    {locator_UIScrollBar_Navi, {locator_UIScrollBar_Navi}},
    {locator_UIScrollWnd, {locator_UIScrollWnd}},
    {locator_UISecondCostumeWnd, {locator_UISecondCostumeWnd}},
    {locator_UISecondPasswordEditWnd, {locator_UISecondPasswordEditWnd}},
    {locator_UISecondPasswordInputWnd, {locator_UISecondPasswordInputWnd}},
    {locator_UISeekPartyListWnd, {locator_UISeekPartyListWnd}},
    {locator_UISeekPartyMBWnd, {locator_UISeekPartyMBWnd}},
    {locator_UISeekPartyWnd, {locator_UISeekPartyWnd}},
    {locator_UISelCharForUServerWnd, {locator_UISelCharForUServerWnd}},
    {locator_UISelectCartWnd, {locator_UISelectCartWnd}},
    {locator_UISelectReplayDataWnd, {locator_UISelectReplayDataWnd}},
    {locator_UISelectServerWnd, {locator_UISelectServerWnd}},
    {locator_UISerachStoreInfoWnd, {locator_UISerachStoreInfoWnd}},
    {locator_UIServerListBox, {locator_UIServerListBox}},
    {locator_UIShortCutWnd, {locator_UIShortCutWnd}},
    {locator_UIShowAndEditTimeCtrl, {locator_UIShowAndEditTimeCtrl}},
    {locator_UIShowInfoWnd, {locator_UIShowInfoWnd}},
    {locator_UIShowWarpWnd, {locator_UIShowWarpWnd}},
    {locator_UISkillDescribeWnd, {locator_UISkillDescribeWnd}},
    {locator_UISkillListWnd, {locator_UISkillListWnd}},
    {locator_UISkillNameChangeWnd, {locator_UISkillNameChangeWnd}},
    {locator_UISkipButton, {locator_UISkipButton}},
    {locator_UISlotBitmap, {locator_UISlotBitmap}},
    {locator_UISlotBmpButton, {locator_UISlotBmpButton}},
    {locator_UISlotForSelectWnd, {locator_UISlotForSelectWnd}},
    {locator_UISlotOfMyStyleWnd, {locator_UISlotOfMyStyleWnd}},
    {locator_UISpellListWnd, {locator_UISpellListWnd}},
    {locator_UISpriteButton, {locator_UISpriteButton}},
    {locator_UIStarPlaceAcceptWnd, {locator_UIStarPlaceAcceptWnd}},
    {locator_UIStaticBitmap, {locator_UIStaticBitmap}},
    {locator_UIStaticItem, {locator_UIStaticItem}},
    {locator_UIStaticItem_Illust, {locator_UIStaticItem_Illust}},
    {locator_UIStaticText, {locator_UIStaticText}},
    {locator_UIStaticText3, {locator_UIStaticText3}},
    {locator_UIStaticText_LF, {locator_UIStaticText_LF}},
    {locator_UIStatusWnd, {locator_UIStatusWnd}},
    {locator_UIStorePasswordInputWnd, {locator_UIStorePasswordInputWnd}},
    {locator_UIString, {locator_UIString}},
    {locator_UIStylingShopButton, {locator_UIStylingShopButton}},
    {locator_UIStylingShopWnd, {locator_UIStylingShopWnd}},
    {locator_UIStylingshopTabParentWnd, {locator_UIStylingshopTabParentWnd}},
    {locator_UISubChatHisWnd, {locator_UISubChatHisWnd}},
    {locator_UISubChatMiniWnd, {locator_UISubChatMiniWnd}},
    {locator_UISwapBitmapButton, {locator_UISwapBitmapButton}},
    {locator_UISwapEquipmentWnd, {locator_UISwapEquipmentWnd}},
    {locator_UITVBitmapButton, {locator_UITVBitmapButton}},
    {locator_UITVTextButton, {locator_UITVTextButton}},
    {locator_UITab, {locator_UITab}},
    {locator_UITabControl, {locator_UITabControl}},
    {locator_UITableBox, {locator_UITableBox}},
    {locator_UITagButton, {locator_UITagButton}},
    {locator_UITagListBox, {locator_UITagListBox}},
    {locator_UITagTextViewer, {locator_UITagTextViewer}},
    {locator_UITalkboxTrapInputWnd, {locator_UITalkboxTrapInputWnd}},
    {locator_LineCutter_UIText, {locator_LineCutter_UIText}},
    {locator_UITextButton, {locator_UITextButton}},
    {locator_UITextViewEX, {locator_UITextViewEX}},
    {locator_UITextViewer, {locator_UITextViewer}},
    {locator_UITextureButton, {locator_UITextureButton}},
    {locator_UITipOfTheDayWnd, {locator_UITipOfTheDayWnd}},
    {locator_UITipboxWnd, {locator_UITipboxWnd}},
    {locator_UIToolTipBitmapWnd, {locator_UIToolTipBitmapWnd}},
    {locator_UIToolTipWnd, {locator_UIToolTipWnd}},
    {locator_UITransBalloonText, {locator_UITransBalloonText}},
    {locator_UITransMenuWnd, {locator_UITransMenuWnd}},
    {locator_UITreeView, {locator_UITreeView}},
    {locator_UITwitterEditCtrl3, {locator_UITwitterEditCtrl3}},
    {locator_UITwitterScrollBar, {locator_UITwitterScrollBar}},
    {locator_UIVerticalNameBalloonText, {locator_UIVerticalNameBalloonText}},
    {locator_UIVideoWnd, {locator_UIVideoWnd}},
    {locator_UIViewCameraInfoWnd, {locator_UIViewCameraInfoWnd}},
    {locator_UIWaitWnd, {locator_UIWaitWnd}},
    {locator_UIWeaponRefineWnd, {locator_UIWeaponRefineWnd}},
    {locator_UIWhisperEditCtrl, {locator_UIWhisperEditCtrl}},
    {locator_UIWhisperWnd, {locator_UIWhisperWnd}},
    {locator_UIWindow, {locator_UIWindow}},
    {locator_UIWordDisplayWnd, {locator_UIWordDisplayWnd}},
    {locator_UIYourItemWnd, {locator_UIYourItemWnd}},
    {locator_UIZeroQuestScrollBar, {locator_UIZeroQuestScrollBar}},
    {locator_UInCashWnd, {locator_UInCashWnd}},
    {locator_UInCash_CallWnd, {locator_UInCash_CallWnd}},
};

const struct ReferenceCast reference_casts[OBJECT_TYPE_COUNT] = {
    {OBJECT_BITMAP_RESOURCE, &type_CRes, &type_CBitmapRes},
    {OBJECT_SPRITE_RESOURCE, &type_CRes, &type_CSprRes},
    {OBJECT_ACTION_RESOURCE, &type_CRes, &type_CActRes},
    {OBJECT_EMBLEM_RESOURCE, &type_CRes, &type_CEmblemRes},
    {OBJECT_GIF_RESOURCE, &type_CRes, &type_CGifRes},
    {OBJECT_PALETTE_RESOURCE, &type_CRes, &type_CPaletteRes},
    {OBJECT_PC, &type_CGameActor, &type_CPc},
    {OBJECT_GAME_ACTOR, &type_CRenderObject, &type_CGameActor},
    {OBJECT_TETRA_BALL_EFFECT, &type_CEffect, &type_CTetraBallEffect},
    {OBJECT_FRAME_WINDOW, &type_UIWindow, &type_UIFrameWnd},
    {OBJECT_BASIC_BUTTONS_WINDOW, &type_UIWindow, &type_UIButtonsOfBasicWnd},
    {OBJECT_PARA_RESULT_WINDOW, &type_UIWindow, &type_UIParaResultWnd},
    {OBJECT_ENTRY_QUEUE_WINDOW, &type_UIWindow, &type_UIEntryQueueWnd},
    {OBJECT_ENTRY_QUEUE_STANDBY_WINDOW, &type_UIWindow, &type_UIEntryQueueStandByWnd},
    {OBJECT_MY_SHOP_WINDOW, &type_UIWindow, &type_UIMerchantItemMyShopWnd},
    {OBJECT_GRAPHICS_SETTINGS_WINDOW, &type_UIWindow, &type_UIGraphicSettingWnd},
    {OBJECT_ITEM_COUNT_WINDOW, &type_UIWindow, &type_UIItemDropCntWnd},
    {OBJECT_PARA_PURCHASE_WINDOW, &type_UIWindow, &type_UIParaItemPurchaseWnd},
    {OBJECT_PARA_SHOP_WINDOW, &type_UIWindow, &type_UIParaItemShopWnd},
};

const uint32_t reference_valid_sources[611] = {
    UINT32_C(128),
    UINT32_C(63),
    UINT32_C(63),
    UINT32_C(192),
    UINT32_C(63),
    UINT32_C(63),
    UINT32_C(63),
    UINT32_C(63),
    UINT32_C(63),
    UINT32_C(256),
    UINT32_C(128),
    UINT32_C(256),
    UINT32_C(63),
    UINT32_C(256),
    UINT32_C(128),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(63),
    UINT32_C(256),
    UINT32_C(128),
    UINT32_C(192),
    UINT32_C(523776),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(192),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(523776),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(128),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(128),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(63),
    UINT32_C(256),
    UINT32_C(128),
    UINT32_C(128),
    UINT32_C(256),
    UINT32_C(63),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(192),
    UINT32_C(63),
    UINT32_C(256),
    UINT32_C(63),
    UINT32_C(128),
    UINT32_C(192),
    UINT32_C(192),
    UINT32_C(256),
    UINT32_C(128),
    UINT32_C(192),
    UINT32_C(192),
    UINT32_C(192),
    UINT32_C(192),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(128),
    UINT32_C(63),
    UINT32_C(128),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(128),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(128),
    UINT32_C(523776),
    UINT32_C(256),
    UINT32_C(128),
    UINT32_C(128),
    UINT32_C(256),
    UINT32_C(192),
    UINT32_C(128),
    UINT32_C(256),
    UINT32_C(63),
    UINT32_C(192),
    UINT32_C(256),
    UINT32_C(192),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(128),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(128),
    UINT32_C(63),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(128),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(523776),
    UINT32_C(192),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(128),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(63),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(523776),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(256),
    UINT32_C(63),
    UINT32_C(256),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
    UINT32_C(523776),
};
