GuiWindowBox      : UiRect + UiTitle
GuiGroupBox       : UiRect + UiText
GuiLine           : UiRect + UiText
GuiPanel          : UiRect + UiText
GuiTabBar         : UiRect + UiItemsArray + UiActiveIndex
GuiScrollPanel    : UiRect + UiText + UiContentRect + UiScroll + UiViewRect
GuiLabel          : UiRect + UiText
GuiButton         : UiRect + UiText
GuiLabelButton    : UiRect + UiText
GuiToggle         : UiRect + UiText + UiValueBool
GuiToggleGroup    : UiRect + UiItemsText + UiActiveIndex
GuiToggleSlider   : UiRect + UiItemsText + UiActiveIndex
GuiCheckBox       : UiRect + UiText + UiValueBool
GuiComboBox       : UiRect + UiItemsText + UiActiveIndex
GuiDropdownBox    : UiRect + UiItemsText + UiActiveIndex + UiEditMode
GuiSpinner        : UiRect + UiText + UiValueInt + UiRangeInt + UiEditMode
GuiValueBox       : UiRect + UiText + UiValueInt + UiRangeInt + UiEditMode
GuiValueBoxFloat  : UiRect + UiText + UiTextBuffer + UiValueFloat + UiEditMode
GuiTextBox        : UiRect + UiTextBuffer + UiEditMode
GuiSlider         : UiRect + UiTextLeft + UiTextRight + UiValueFloat + UiRangeFloat
GuiSliderBar      : UiRect + UiTextLeft + UiTextRight + UiValueFloat + UiRangeFloat
GuiProgressBar    : UiRect + UiTextLeft + UiTextRight + UiValueFloat + UiRangeFloat
GuiStatusBar      : UiRect + UiText
GuiDummyRec       : UiRect + UiText
GuiGrid           : UiRect + UiText + UiGridSpec + UiMouseCell
GuiListView       : UiRect + UiItemsText + UiScrollIndex + UiActiveIndex
GuiListViewEx     : UiRect + UiItemsArray + UiScrollIndex + UiActiveIndex + UiFocusIndex
GuiMessageBox     : UiRect + UiTitle + UiMessage + UiButtons (+ UiResult)
GuiTextInputBox   : UiRect + UiTitle + UiMessage + UiButtons + UiTextBuffer + UiSecretView (+ UiResult)
GuiColorPicker    : UiRect + UiText + UiColor
GuiColorPanel     : UiRect + UiText + UiColor
GuiColorBarAlpha  : UiRect + UiText + UiValueFloat
GuiColorBarHue    : UiRect + UiText + UiValueFloat
GuiColorPickerHSV : UiRect + UiText + UiColorHSV
GuiColorPanelHSV  : UiRect + UiText + UiColorHSV
