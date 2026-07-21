#include "soundboard-dock.h"

#include <QtWidgets/QGroupBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QMessageBox>
#include <util/platform.h>

struct HotkeyCbData {
	SoundboardDock *dock;
	int index;
};

static void soundboard_hotkey_callback(void *data, obs_hotkey_id id,
				       obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	HotkeyCbData *cb = (HotkeyCbData *)data;
	if (!cb || !cb->dock)
		return;

	SoundboardDock *dock = cb->dock;
	int idx = cb->index;

	if (idx < 0 || idx >= (int)dock->_entries.size())
		return;

	if (pressed)
		dock->playStopSound(idx);
}

static void register_entry_hotkey(SoundboardDock *dock, int index)
{
	SoundEntry &entry = dock->_entries[index];

	if (entry.hotkeyUserData) {
		delete (HotkeyCbData *)entry.hotkeyUserData;
		entry.hotkeyUserData = nullptr;
	}
	if (entry.hotkeyId)
		obs_hotkey_unregister(entry.hotkeyId);

	char hotkeyName[256];
	snprintf(hotkeyName, sizeof(hotkeyName), "Soundboard.Play.%d.%s",
		 index, entry.name.c_str());

	HotkeyCbData *cb = new HotkeyCbData();
	cb->dock = dock;
	cb->index = index;
	entry.hotkeyUserData = cb;

	entry.hotkeyId = obs_hotkey_register_frontend(
		hotkeyName, entry.name.c_str(),
		soundboard_hotkey_callback, cb);
}

static void unregister_entry_hotkey(SoundEntry &entry)
{
	if (entry.hotkeyUserData) {
		delete (HotkeyCbData *)entry.hotkeyUserData;
		entry.hotkeyUserData = nullptr;
	}
	if (entry.hotkeyId) {
		obs_hotkey_unregister(entry.hotkeyId);
		entry.hotkeyId = 0;
	}
}

obs_source_t *SoundboardDock::create_media_source(const std::string &filepath)
{
	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "local_file", filepath.c_str());
	obs_data_set_bool(settings, "is_local_file", true);
	obs_data_set_bool(settings, "looping", false);
	obs_data_set_bool(settings, "restart_on_activate", false);

	obs_source_t *source = obs_source_create("ffmpeg_source",
						 filepath.c_str(),
						 settings, nullptr);
	obs_data_release(settings);

	return source;
}

SoundboardDock::SoundboardDock(QWidget *parent) : QWidget(parent)
{
	setObjectName("soundboardDock");
	setWindowTitle(obs_module_text("Soundboard"));

	createUI();
	loadSettings();

	_mediaTimer = new QTimer(this);
	connect(_mediaTimer, &QTimer::timeout, this,
		&SoundboardDock::checkMediaEnded);
	_mediaTimer->start(100);
}

SoundboardDock::~SoundboardDock()
{
	saveSettings();

	for (auto &entry : _entries) {
		unregister_entry_hotkey(entry);
		if (entry.mediaSource)
			obs_source_release(entry.mediaSource);
	}
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
	entry.playing = false;

	entry.mediaSource = create_media_source(entry.filepath);
	if (!entry.mediaSource) {
		QMessageBox::warning(this,
				     obs_module_text("Error"),
				     obs_module_text("FailedToLoadAudio"));
		return;
	}

	obs_source_set_volume(entry.mediaSource, entry.volume / 100.0f);

	_entries.push_back(entry);
	int idx = (int)_entries.size() - 1;

	register_entry_hotkey(this, idx);
	updateEntryUI(idx);
}

void SoundboardDock::removeSound(int index)
{
	if (index < 0 || index >= (int)_entries.size())
		return;

	unregister_entry_hotkey(_entries[index]);
	if (_entries[index].mediaSource)
		obs_source_release(_entries[index].mediaSource);

	if (_entriesLayout->count() > index) {
		QLayoutItem *item = _entriesLayout->takeAt(index);
		if (item->widget())
			item->widget()->deleteLater();
		delete item;
	}

	_entries.erase(_entries.begin() + index);

	for (int i = index; i < (int)_entries.size(); i++) {
		register_entry_hotkey(this, i);
		if (_entriesLayout->count() > i) {
			QLayoutItem *item = _entriesLayout->takeAt(i);
			if (item->widget())
				item->widget()->deleteLater();
			delete item;
		}
		updateEntryUI(i);
	}
}

void SoundboardDock::playStopSound(int index)
{
	if (index < 0 || index >= (int)_entries.size())
		return;

	auto &entry = _entries[index];
	if (!entry.mediaSource)
		return;

	if (entry.playing) {
		obs_source_media_stop(entry.mediaSource);
		entry.playing = false;
		if (entry.playButton)
			entry.playButton->setChecked(false);
	} else {
		obs_source_media_restart(entry.mediaSource);
		entry.playing = true;
		if (entry.playButton)
			entry.playButton->setChecked(true);
	}
}

void SoundboardDock::setVolume(int index, int volume)
{
	if (index < 0 || index >= (int)_entries.size())
		return;

	auto &entry = _entries[index];
	entry.volume = volume;
	if (entry.mediaSource)
		obs_source_set_volume(entry.mediaSource, volume / 100.0f);
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

	entry.nameLabel =
		new QLabel(QString::fromStdString(entry.name), frame);
	entry.nameLabel->setStyleSheet("font-weight: bold;");
	frameLayout->addWidget(entry.nameLabel);

	QHBoxLayout *controlRow = new QHBoxLayout();

	entry.playButton =
		new QPushButton(obs_module_text("Play"), frame);
	entry.playButton->setCheckable(true);
	entry.playButton->setChecked(entry.playing);
	entry.playButton->connect(entry.playButton, &QPushButton::clicked,
				  this, [this, index]() {
					  playStopSound(index);
				  });
	controlRow->addWidget(entry.playButton);

	entry.volumeSlider = new QSlider(Qt::Horizontal, frame);
	entry.volumeSlider->setRange(0, 100);
	entry.volumeSlider->setValue(entry.volume);
	entry.volumeSlider->setMaximumWidth(120);
	entry.volumeSlider->connect(entry.volumeSlider,
				    &QSlider::valueChanged, this,
				    [this, index](int val) {
					    setVolume(index, val);
				    });
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
	entry.hotkeyLabel->setStyleSheet(
		"color: gray; font-size: 11px;");
	frameLayout->addWidget(entry.hotkeyLabel);

	_entriesLayout->insertWidget(index, frame);
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
		entry.playing = false;

		if (!entry.filepath.empty())
			entry.mediaSource = create_media_source(entry.filepath);

		if (entry.mediaSource)
			obs_source_set_volume(entry.mediaSource,
					      entry.volume / 100.0f);

		_entries.push_back(entry);
		int idx = (int)_entries.size() - 1;
		register_entry_hotkey(this, idx);
		updateEntryUI(idx);
	}
	settings.endArray();
}

void SoundboardDock::checkMediaEnded()
{
	for (auto &entry : _entries) {
		if (!entry.playing || !entry.mediaSource)
			continue;
		enum obs_media_state state =
			obs_source_media_get_state(entry.mediaSource);
		if (state == OBS_MEDIA_STATE_ENDED) {
			entry.playing = false;
			if (entry.playButton)
				entry.playButton->setChecked(false);
		}
	}
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
