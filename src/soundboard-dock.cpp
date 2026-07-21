#include "soundboard-dock.h"

#include <QtWidgets/QGroupBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QMessageBox>
#include <util/platform.h>

static void soundboard_hotkey_callback(void *data, obs_hotkey_id id,
				       obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	SoundEntry *entry = (SoundEntry *)data;
	if (!entry)
		return;

	if (pressed) {
		if (!entry->playing) {
			entry->playPosition = 0;
			entry->playing = true;
		} else {
			entry->playing = false;
		}
	}
}

SoundboardDock::SoundboardDock(QWidget *parent) : QWidget(parent)
{
	setObjectName("soundboardDock");
	setWindowTitle(obs_module_text("Soundboard"));

	createUI();
	loadSettings();

	_playbackTimer = new QTimer(this);
	connect(_playbackTimer, &QTimer::timeout, this,
		&SoundboardDock::onPlaybackTick);
	_playbackTimer->start(30);
}

SoundboardDock::~SoundboardDock()
{
	saveSettings();
	for (auto &entry : _entries)
		obs_hotkey_unregister(entry.hotkeyId);
}

void SoundboardDock::createUI()
{
	_mainLayout = new QVBoxLayout(this);
	_mainLayout->setContentsMargins(8, 8, 8, 8);

	QPushButton *addBtn =
		new QPushButton(obs_module_text("AddSound"), this);
	connect(addBtn, &QPushButton::clicked, this,
		&SoundboardDock::addSound);
	_mainLayout->addWidget(addBtn);

	_scrollArea = new QScrollArea(this);
	_scrollArea->setWidgetResizable(true);
	_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	_scrollContent = new QWidget();
	_entriesLayout = new QVBoxLayout(_scrollContent);
	_entriesLayout->setAlignment(Qt::AlignTop);
	_scrollArea->setWidget(_scrollContent);

	_mainLayout->addWidget(_scrollArea);
}

void SoundboardDock::addSound()
{
	QString file = QFileDialog::getOpenFileName(
		this, obs_module_text("SelectAudioFile"), QString(),
		"Audio Files (*.mp3 *.wav *.ogg *.flac *.aac *.m4a);;All Files (*)");

	if (file.isEmpty())
		return;

	SoundEntry entry;
	entry.name = QFileInfo(file).baseName().toStdString();
	entry.filepath = file.toStdString();
	entry.volume = 80;
	entry.playPosition = 0;
	entry.playing = false;

	if (!entry.decoder.load(entry.filepath)) {
		QMessageBox::warning(this,
				     obs_module_text("Error"),
				     obs_module_text("FailedToLoadAudio"));
		return;
	}

	char hotkeyName[256];
	snprintf(hotkeyName, sizeof(hotkeyName), "Soundboard.Play.%s",
		 entry.name.c_str());

	entry.hotkeyId = obs_hotkey_register_frontend(
		hotkeyName, entry.name.c_str(),
		soundboard_hotkey_callback, &entry);

	_entries.push_back(entry);
	int idx = (int)_entries.size() - 1;
	updateEntryUI(idx);
}

void SoundboardDock::removeSound(int index)
{
	if (index < 0 || index >= (int)_entries.size())
		return;

	obs_hotkey_unregister(_entries[index].hotkeyId);

	if (_entriesLayout->count() > index) {
		QLayoutItem *item = _entriesLayout->takeAt(index);
		if (item->widget())
			item->widget()->deleteLater();
		delete item;
	}

	_entries.erase(_entries.begin() + index);
}

void SoundboardDock::playStopSound(int index)
{
	if (index < 0 || index >= (int)_entries.size())
		return;

	auto &entry = _entries[index];
	if (entry.playing) {
		entry.playing = false;
	} else {
		entry.playPosition = 0;
		entry.playing = true;
	}
}

void SoundboardDock::setVolume(int index, int volume)
{
	if (index < 0 || index >= (int)_entries.size())
		return;
	_entries[index].volume = volume;
}

void SoundboardDock::updateEntryUI(int index)
{
	if (index < 0 || index >= (int)_entries.size())
		return;

	auto &entry = _entries[index];

	QFrame *frame = new QFrame(_scrollContent);
	frame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

	QVBoxLayout *frameLayout = new QVBoxLayout(frame);
	frameLayout->setContentsMargins(6, 6, 6, 6);

	entry.nameLabel = new QLabel(QString::fromStdString(entry.name), frame);
	entry.nameLabel->setStyleSheet("font-weight: bold;");
	frameLayout->addWidget(entry.nameLabel);

	QHBoxLayout *controlRow = new QHBoxLayout();

	entry.playButton = new QPushButton(
		obs_module_text("Play"), frame);
	entry.playButton->setCheckable(true);
	entry.playButton->connect(entry.playButton, &QPushButton::clicked, this,
				  [this, index]() { playStopSound(index); });
	controlRow->addWidget(entry.playButton);

	entry.volumeSlider = new QSlider(Qt::Horizontal, frame);
	entry.volumeSlider->setRange(0, 100);
	entry.volumeSlider->setValue(entry.volume);
	entry.volumeSlider->setMaximumWidth(120);
	entry.volumeSlider->connect(entry.volumeSlider, &QSlider::valueChanged,
				    this,
				    [this, index](int val) { setVolume(index, val); });
	controlRow->addWidget(entry.volumeSlider);

	QPushButton *removeBtn = new QPushButton("X", frame);
	removeBtn->setFixedWidth(28);
	removeBtn->setToolTip(obs_module_text("RemoveSound"));
	removeBtn->connect(removeBtn, &QPushButton::clicked, this,
			   [this, index]() { removeSound(index); });
	controlRow->addWidget(removeBtn);

	frameLayout->addLayout(controlRow);

	entry.hotkeyLabel = new QLabel(
		QString("Hotkey: ") +
			QString::fromStdString(entry.name),
		frame);
	entry.hotkeyLabel->setStyleSheet("color: gray; font-size: 11px;");
	frameLayout->addWidget(entry.hotkeyLabel);

	_entriesLayout->insertWidget(index, frame);
}

void SoundboardDock::onPlaybackTick()
{
	for (auto &entry : _entries) {
		if (!entry.playing || !entry.decoder.loaded())
			continue;
		const AudioSamples &samples = entry.decoder.samples();
		size_t totalFrames = samples.total_frames;
		if (totalFrames == 0)
			continue;
		entry.playPosition =
			(entry.playPosition + 1) % totalFrames;
	}
}

void SoundboardDock::saveSettings()
{
	QSettings settings("obs-soundboard-censor", "Soundboard");
	settings.beginGroup("sounds");
	settings.beginWriteArray("entries");
	for (int i = 0; i < (int)_entries.size(); i++) {
		settings.setArrayIndex(i);
		settings.setValue("name",
				  QString::fromStdString(_entries[i].name));
		settings.setValue("filepath",
				  QString::fromStdString(_entries[i].filepath));
		settings.setValue("volume", _entries[i].volume);
	}
	settings.endArray();
	settings.endGroup();
}

void SoundboardDock::loadSettings()
{
	QSettings settings("obs-soundboard-censor", "Soundboard");
	settings.beginGroup("sounds");
	int size = settings.beginReadArray("entries");
	for (int i = 0; i < size; i++) {
		settings.setArrayIndex(i);
		SoundEntry entry;
		entry.name = settings.value("name").toString().toStdString();
		entry.filepath =
			settings.value("filepath").toString().toStdString();
		entry.volume = settings.value("volume", 80).toInt();
		entry.playPosition = 0;
		entry.playing = false;

		if (!entry.filepath.empty())
			entry.decoder.load(entry.filepath);

		char hotkeyName[256];
		snprintf(hotkeyName, sizeof(hotkeyName),
			 "Soundboard.Play.%s", entry.name.c_str());

		entry.hotkeyId = obs_hotkey_register_frontend(
			hotkeyName, entry.name.c_str(),
			soundboard_hotkey_callback, &entry);

		_entries.push_back(entry);
		updateEntryUI(i);
	}
	settings.endArray();
}

SoundEntry &SoundboardDock::entryAt(int index)
{
	static SoundEntry dummy;
	if (index < 0 || index >= (int)_entries.size())
		return dummy;
	return _entries[index];
}

const std::string &SoundboardDock::entryName(int index) const
{
	static std::string empty;
	if (index < 0 || index >= (int)_entries.size())
		return empty;
	return _entries[index].name;
}

void SoundboardDock::registerHotkey(int index, obs_hotkey_id id)
{
	if (index >= 0 && index < (int)_entries.size())
		_entries[index].hotkeyId = id;
}

void SoundboardDock::unregisterHotkey(int index)
{
	if (index >= 0 && index < (int)_entries.size())
		_entries[index].hotkeyId = 0;
}
