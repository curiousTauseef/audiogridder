/*
 * Copyright (c) 2020 Andreas Pohl
 * Licensed under MIT (https://github.com/apohl79/audiogridder/blob/master/COPYING)
 *
 * Author: Andreas Pohl
 */

#include "ServerSettingsWindow.hpp"
#include "App.hpp"

namespace e47 {

ServerSettingsWindow::ServerSettingsWindow(App* app)
    : DocumentWindow("Server Settings",
                     LookAndFeel::getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId),
                     DocumentWindow::closeButton),
      m_app(app) {
    setUsingNativeTitleBar(true);

    int totalWidth = 500;
    int totalHeight = 80;
    int borderLR = 15;  // left/right border
    int borderTB = 15;  // top/botton border
    int rowHeight = 35;

    int fieldWidth = 50;
    int wideFieldWidth = 250;
    int fieldHeight = 25;
    int labelWidth = 250;
    int labelHeight = 30;
    int checkBoxWidth = 25;
    int checkBoxHeight = 25;
    int largeFieldRows = 2;
    int largeFieldWidth = 250;
    int largeFieldHeight = largeFieldRows * rowHeight - 10;

    int saveButtonWidth = 125;
    int saveButtonHeight = 30;

    auto getLabelBounds = [&](int r) {
        return Rectangle<int>(borderLR, borderTB + r * rowHeight, labelWidth, labelHeight);
    };
    auto getFieldBounds = [&](int r) {
        return Rectangle<int>(totalWidth - fieldWidth - borderLR, borderTB + r * rowHeight + 3, fieldWidth,
                              fieldHeight);
    };
    auto getWideFieldBounds = [&](int r) {
        return Rectangle<int>(totalWidth - wideFieldWidth - borderLR, borderTB + r * rowHeight + 3, wideFieldWidth,
                              fieldHeight);
    };
    auto getCheckBoxBounds = [&](int r) {
        return Rectangle<int>(totalWidth - checkBoxWidth - borderLR, borderTB + r * rowHeight + 3, checkBoxWidth,
                              checkBoxHeight);
    };
    auto getLargeFieldBounds = [&](int r) {
        return Rectangle<int>(totalWidth - largeFieldWidth - borderLR, borderTB + r * rowHeight + 3, largeFieldWidth,
                              largeFieldHeight);
    };

    int row = 0;
    String tmpStr;

    auto label = std::make_unique<Label>();
    label->setText("Server Name:", NotificationType::dontSendNotification);
    label->setBounds(getLabelBounds(row));
    addChildAndSetID(label.get(), "lbl");
    m_components.push_back(std::move(label));

    m_nameText.setText(m_app->getServer().getName());
    m_nameText.setBounds(getWideFieldBounds(row));
    addChildAndSetID(&m_nameText, "name");

    row++;

    label = std::make_unique<Label>();
    label->setText("Server ID:", NotificationType::dontSendNotification);
    label->setBounds(getLabelBounds(row));
    addChildAndSetID(label.get(), "lbl");
    m_components.push_back(std::move(label));

    String id;
    id << m_app->getServer().getId();
    m_idText.setText(id);
    m_idText.setBounds(getFieldBounds(row));
    addChildAndSetID(&m_idText, "id");

    row++;

#ifdef JUCE_MAC
    label = std::make_unique<Label>();
    label->setText("AudioUnit Support:", NotificationType::dontSendNotification);
    label->setBounds(getLabelBounds(row));
    addChildAndSetID(label.get(), "lbl");
    m_components.push_back(std::move(label));

    m_auSupport.setBounds(getCheckBoxBounds(row));
    m_auSupport.setToggleState(m_app->getServer().getEnableAU(), NotificationType::dontSendNotification);
    addChildAndSetID(&m_auSupport, "au");

    row++;
#endif

    label = std::make_unique<Label>();
    label->setText("VST3 Support:", NotificationType::dontSendNotification);
    label->setBounds(getLabelBounds(row));
    addChildAndSetID(label.get(), "lbl");
    m_components.push_back(std::move(label));

    m_vst3Support.setBounds(getCheckBoxBounds(row));
    m_vst3Support.setToggleState(m_app->getServer().getEnableVST3(), NotificationType::dontSendNotification);
    addChildAndSetID(&m_vst3Support, "vst");

    row++;

    label = std::make_unique<Label>();
    tmpStr = "VST3 Custom Folders";
    tmpStr << newLine << "(one folder per line):";
    label->setText(tmpStr, NotificationType::dontSendNotification);
    label->setBounds(getLabelBounds(row));
    addChildAndSetID(label.get(), "lbl");
    m_components.push_back(std::move(label));

    m_vst3Folders.setBounds(getLargeFieldBounds(row));
    m_vst3Folders.setMultiLine(true, false);
    m_vst3Folders.setReturnKeyStartsNewLine(true);
    addChildAndSetID(&m_vst3Folders, "vst3fold");

    tmpStr = "";
    for (auto& folder : m_app->getServer().getVST3Folders()) {
        tmpStr << folder << newLine;
    }
    m_vst3Folders.setText(tmpStr);

    row += largeFieldRows;

    label = std::make_unique<Label>();
    label->setText("VST2 Support:", NotificationType::dontSendNotification);
    label->setBounds(getLabelBounds(row));
    addChildAndSetID(label.get(), "lbl");
    m_components.push_back(std::move(label));

    m_vst2Support.setBounds(getCheckBoxBounds(row));
    m_vst2Support.setToggleState(m_app->getServer().getEnableVST2(), NotificationType::dontSendNotification);
    addChildAndSetID(&m_vst2Support, "vst2");

    row++;

    label = std::make_unique<Label>();
    tmpStr = "VST2 Custom Folders";
    tmpStr << newLine << "(one folder per line):";
    label->setText(tmpStr, NotificationType::dontSendNotification);
    label->setBounds(getLabelBounds(row));
    addChildAndSetID(label.get(), "lbl");
    m_components.push_back(std::move(label));

    m_vst2Folders.setBounds(getLargeFieldBounds(row));
    m_vst2Folders.setMultiLine(true, false);
    m_vst2Folders.setReturnKeyStartsNewLine(true);
    addChildAndSetID(&m_vst2Folders, "vst2fold");

    tmpStr = "";
    for (auto& folder : m_app->getServer().getVST2Folders()) {
        tmpStr << folder << newLine;
    }
    m_vst2Folders.setText(tmpStr);

    row += largeFieldRows;

    label = std::make_unique<Label>();
    label->setText("Screen Capturing Mode:", NotificationType::dontSendNotification);
    label->setBounds(getLabelBounds(row));
    addChildAndSetID(label.get(), "lbl");
    m_components.push_back(std::move(label));

    m_screenCapturingMode.setBounds(getWideFieldBounds(row));
    m_screenCapturingMode.addItem("FFmpeg", 1);
    m_screenCapturingMode.addItem("Legacy", 2);
    m_screenCapturingMode.addItem("Disabled", 3);
    int mode = 1;
    if (m_app->getServer().getScreenCapturingOff()) {
        mode = 3;
    } else if (!m_app->getServer().getScreenCapturingFFmpeg()) {
        mode = 2;
    }
    m_screenCapturingMode.setSelectedId(mode, NotificationType::dontSendNotification);
    m_screenCapturingMode.onChange = [this] {
        if (m_screenCapturingMode.getSelectedId() != 2) {
            m_screenDiffDetectionLbl.setAlpha(0.5);
            m_screenDiffDetection.setEnabled(false);
            m_screenDiffDetection.setAlpha(0.5);
            m_screenJpgQualityLbl.setAlpha(0.5);
            m_screenJpgQuality.setEnabled(false);
            m_screenJpgQuality.setAlpha(0.5);
        } else {
            m_screenDiffDetectionLbl.setAlpha(1);
            m_screenDiffDetection.setEnabled(true);
            m_screenDiffDetection.setAlpha(1);
            m_screenJpgQualityLbl.setAlpha(1);
            m_screenJpgQuality.setEnabled(true);
            m_screenJpgQuality.setAlpha(1);
            if (nullptr != m_screenDiffDetection.onClick) {
                m_screenDiffDetection.onClick();
            }
        }
    };
    m_screenCapturingMode.onChange();
    addChildAndSetID(&m_screenCapturingMode, "captmode");

    row++;

    label = std::make_unique<Label>();
    m_screenDiffDetectionLbl.setText("Screen Capture Diff Detection:", NotificationType::dontSendNotification);
    m_screenDiffDetectionLbl.setBounds(getLabelBounds(row));
    addChildAndSetID(&m_screenDiffDetectionLbl, "lbl");

    m_screenDiffDetection.setBounds(getCheckBoxBounds(row));
    m_screenDiffDetection.setToggleState(m_app->getServer().getScreenDiffDetection(),
                                         NotificationType::dontSendNotification);
    m_screenDiffDetection.onClick = [this] {
        if (m_screenCapturingMode.getSelectedId() == 2) {
            if (m_screenDiffDetection.getToggleState()) {
                m_screenJpgQualityLbl.setAlpha(0.5);
                m_screenJpgQuality.setEnabled(false);
                m_screenJpgQuality.setAlpha(0.5);
            } else {
                m_screenJpgQualityLbl.setAlpha(1);
                m_screenJpgQuality.setEnabled(true);
                m_screenJpgQuality.setAlpha(1);
            }
        }
    };
    m_screenDiffDetection.onClick();
    addChildAndSetID(&m_screenDiffDetection, "diff");

    row++;

    m_screenJpgQualityLbl.setText("Screen Capture Quality (0.1-1.0):", NotificationType::dontSendNotification);
    m_screenJpgQualityLbl.setBounds(getLabelBounds(row));
    addChildAndSetID(&m_screenJpgQualityLbl, "lbl");

    String q;
    q << m_app->getServer().getScreenQuality();
    m_screenJpgQuality.setText(q);
    m_screenJpgQuality.setBounds(getFieldBounds(row));
    addChildAndSetID(&m_screenJpgQuality, "qual");

    row++;

    totalHeight += row * rowHeight;

    m_saveButton.setButtonText("Save");
    m_saveButton.setBounds(totalWidth / 2 - saveButtonWidth / 2, totalHeight - borderTB - saveButtonHeight,
                           saveButtonWidth, saveButtonHeight);
    m_saveButton.onClick = [this, app] {
        auto appCpy = app;
        appCpy->getServer().setId(m_idText.getText().getIntValue());
        appCpy->getServer().setName(m_nameText.getText());
        appCpy->getServer().setEnableAU(m_auSupport.getToggleState());
        appCpy->getServer().setEnableVST3(m_vst3Support.getToggleState());
        appCpy->getServer().setEnableVST2(m_vst2Support.getToggleState());
        switch (m_screenCapturingMode.getSelectedId()) {
            case 1:
                appCpy->getServer().setScreenCapturingFFmpeg(true);
                appCpy->getServer().setScreenCapturingOff(false);
                break;
            case 2:
                appCpy->getServer().setScreenCapturingFFmpeg(false);
                appCpy->getServer().setScreenCapturingOff(false);
                break;
            case 3:
                appCpy->getServer().setScreenCapturingFFmpeg(false);
                appCpy->getServer().setScreenCapturingOff(true);
                break;
        }
        appCpy->getServer().setScreenDiffDetection(m_screenDiffDetection.getToggleState());
        float qual = m_screenJpgQuality.getText().getFloatValue();
        if (qual < 0.1) {
            qual = 0.1f;
        } else if (qual > 1) {
            qual = 1.0f;
        }
        appCpy->getServer().setScreenQuality(qual);
        if (m_vst3Folders.getText().length() > 0) {
            appCpy->getServer().setVST3Folders(StringArray::fromLines(m_vst3Folders.getText()));
        }
        if (m_vst2Folders.getText().length() > 0) {
            appCpy->getServer().setVST2Folders(StringArray::fromLines(m_vst2Folders.getText()));
        }
        appCpy->getServer().saveConfig();
        appCpy->hideServerSettings();
        appCpy->restartServer();
    };
    addChildAndSetID(&m_saveButton, "save");

    setResizable(false, false);
    centreWithSize(totalWidth, totalHeight);
    setVisible(true);
}

void ServerSettingsWindow::closeButtonPressed() { m_app->hideServerSettings(); }

}  // namespace e47
