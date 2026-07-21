#pragma once

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <windows.h>

#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSlider>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QFileDialog>
#include <QtCore/QTimer>
#include <QtCore/QSettings>
#include <QtCore/QFileInfo>

#include <string>
#include <vector>

#include "audio-decoder.h"

struct SoundEntry {
	std::string name;
	std::string filepath;
	int volume = 80;
	QPushButton *playButton = nullptr;
	QSlider *volumeSlider = nullptr;
	QLabel *nameLabel = nullptr;
	QLabel *hotkeyLabel = nullptr;
	obs_hotkey_id hotkeyId = 0;
	AudioDecoder decoder;
	size_t playPosition = 0;
	bool playing = false;
};

class SoundboardDock : public QWidget {
	Q_OBJECT

public:
	SoundboardDock(QWidget *parent = nullptr);
	~SoundboardDock();

	void saveSettings();
	void loadSettings();

	int entryCount() const { return (int)_entries.size(); }
	SoundEntry &entryAt(int index);
	const std::string &entryName(int index) const;

	void registerHotkey(int index, obs_hotkey_id id);
	void unregisterHotkey(int index);

	const std::vector<SoundEntry> &entries() const { return _entries; }

private slots:
	void addSound();
	void removeSound(int index);
	void playStopSound(int index);
	void setVolume(int index, int volume);

private:
	void createUI();
	void updateEntryUI(int index);
	void onPlaybackTick();

	QVBoxLayout *_mainLayout = nullptr;
	QScrollArea *_scrollArea = nullptr;
	QWidget *_scrollContent = nullptr;
	QVBoxLayout *_entriesLayout = nullptr;
	std::vector<SoundEntry> _entries;
	QTimer *_playbackTimer = nullptr;
};
