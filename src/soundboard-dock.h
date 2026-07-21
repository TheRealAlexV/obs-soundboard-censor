#pragma once

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
#include <functional>

#include "audio-decoder.h"

typedef unsigned long long sb_hotkey_id;

struct SBSoundEntry {
	std::string name;
	std::string filepath;
	int volume = 80;
	QPushButton *playButton = nullptr;
	QSlider *volumeSlider = nullptr;
	QLabel *nameLabel = nullptr;
	QLabel *hotkeyLabel = nullptr;
	sb_hotkey_id hotkeyId = 0;
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

	void registerHotkey(int index, sb_hotkey_id id);
	void unregisterHotkey(int index);
	int entryCount() const;
	const std::string &entryName(int index) const;

	std::function<void(int, bool)> onHotkeyPressed;

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
	std::vector<SBSoundEntry> _entries;
	QTimer *_playbackTimer = nullptr;
};

extern "C" QWidget *create_soundboard_dock(QWidget *parent);
extern "C" SoundboardDock *get_soundboard_dock(QWidget *w);
extern "C" int soundboard_get_entry_count(SoundboardDock *dock);
extern "C" const char *soundboard_get_entry_name(SoundboardDock *dock, int index);
