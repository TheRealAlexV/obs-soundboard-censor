#include "soundboard-dock.h"

#include <QtWidgets/QGroupBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QMessageBox>

SoundboardDock::SoundboardDock(QWidget *parent) : QWidget(parent)
{
	setObjectName("soundboardDock");
	setWindowTitle("Soundboard");

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
}

void SoundboardDock::createUI()
{
	_mainLayout = new QVBoxLayout(this);
	_mainLayout->setContentsMargins(8, 8, 8, 8);

	QPushButton *addBtn = new QPushButton("Add Sound", this);
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
		this, "Select Audio File", QString(),
		"Audio Files (*.mp3 *.wav *.ogg *.flac *.aac *.m4a);;All Files (*)");

	if (file.isEmpty())
		return;

	SBSoundEntry entry;
	entry.name = QFileInfo(file).baseName().toStdString();
	entry.filepath = file.toStdString();
	entry.volume = 80;
	entry.playPosition = 0;
	entry.playing = false;

	if (!entry.decoder.load(entry.filepath)) {
		QMessageBox::warning(this, "Error", "Failed to load audio file");
		return;
	}

	_entries.push_back(entry);
	int idx = (int)_entries.size() - 1;
	updateEntryUI(idx);
}

void SoundboardDock::removeSound(int index)
{
	if (index < 0 || index >= (int)_entries.size())
		return;

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

	entry.playButton = new QPushButton("Play", frame);
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
	removeBtn->setToolTip("Remove Sound");
	removeBtn->connect(removeBtn, &QPushButton::clicked, this,
			   [this, index]() { removeSound(index); });
	controlRow->addWidget(removeBtn);

	frameLayout->addLayout(controlRow);

	entry.hotkeyLabel = new QLabel(
		QString("Hotkey: ") + QString::fromStdString(entry.name), frame);
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
		entry.playPosition = (entry.playPosition + 1) % totalFrames;
	}
}

void SoundboardDock::saveSettings()
{
	QSettings settings("obs-soundboard-censor", "Soundboard");
	settings.beginGroup("sounds");
	settings.beginWriteArray("entries");
	for (int i = 0; i < (int)_entries.size(); i++) {
		settings.setArrayIndex(i);
		settings.setValue("name", QString::fromStdString(_entries[i].name));
		settings.setValue("filepath", QString::fromStdString(_entries[i].filepath));
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
		SBSoundEntry entry;
		entry.name = settings.value("name").toString().toStdString();
		entry.filepath = settings.value("filepath").toString().toStdString();
		entry.volume = settings.value("volume", 80).toInt();
		entry.playPosition = 0;
		entry.playing = false;

		if (!entry.filepath.empty())
			entry.decoder.load(entry.filepath);

		_entries.push_back(entry);
		updateEntryUI(i);
	}
	settings.endArray();
}

void SoundboardDock::registerHotkey(int index, sb_hotkey_id id)
{
	if (index >= 0 && index < (int)_entries.size())
		_entries[index].hotkeyId = id;
}

void SoundboardDock::unregisterHotkey(int index)
{
	if (index >= 0 && index < (int)_entries.size())
		_entries[index].hotkeyId = 0;
}

int SoundboardDock::entryCount() const
{
	return (int)_entries.size();
}

const std::string &SoundboardDock::entryName(int index) const
{
	static std::string empty;
	if (index < 0 || index >= (int)_entries.size())
		return empty;
	return _entries[index].name;
}

QWidget *create_soundboard_dock(QWidget *parent)
{
	return new SoundboardDock(parent);
}

SoundboardDock *get_soundboard_dock(QWidget *w)
{
	return dynamic_cast<SoundboardDock *>(w);
}

int soundboard_get_entry_count(SoundboardDock *dock)
{
	if (!dock) return 0;
	return dock->entryCount();
}

const char *soundboard_get_entry_name(SoundboardDock *dock, int index)
{
	if (!dock) return nullptr;
	const std::string &name = dock->entryName(index);
	return name.empty() ? nullptr : name.c_str();
}
