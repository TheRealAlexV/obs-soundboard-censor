#include "audio-decoder.h"

#include <windows.h>
#include <obs-module.h>

#ifdef _WIN32
#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#ifdef __cplusplus
}
#endif
#endif

AudioDecoder::AudioDecoder() {}

AudioDecoder::~AudioDecoder()
{
	unload();
}

bool AudioDecoder::load(const std::string &filepath)
{
	unload();

#ifdef _WIN32
	AVFormatContext *fmtCtx = nullptr;
	if (avformat_open_input(&fmtCtx, filepath.c_str(), nullptr, nullptr) < 0) {
		blog(LOG_WARNING, "[obs-soundboard-censor] Failed to open audio file: %s",
		     filepath.c_str());
		return false;
	}

	if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
		avformat_close_input(&fmtCtx);
		return false;
	}

	int audioStreamIndex = -1;
	for (unsigned int i = 0; i < fmtCtx->nb_streams; i++) {
		if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audioStreamIndex = i;
			break;
		}
	}
	if (audioStreamIndex < 0) {
		avformat_close_input(&fmtCtx);
		return false;
	}

	AVCodecParameters *codecPar = fmtCtx->streams[audioStreamIndex]->codecpar;
	const AVCodec *codec = avcodec_find_decoder(codecPar->codec_id);
	if (!codec) {
		avformat_close_input(&fmtCtx);
		return false;
	}

	AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
	if (!codecCtx) {
		avformat_close_input(&fmtCtx);
		return false;
	}

	if (avcodec_parameters_to_context(codecCtx, codecPar) < 0) {
		avcodec_free_context(&codecCtx);
		avformat_close_input(&fmtCtx);
		return false;
	}

	if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
		avcodec_free_context(&codecCtx);
		avformat_close_input(&fmtCtx);
		return false;
	}

	SwrContext *swr = nullptr;
	AVChannelLayout outLayout = AV_CHANNEL_LAYOUT_STEREO;
	AVSampleFormat outFormat = AV_SAMPLE_FMT_FLTP;
	int outSampleRate = codecCtx->sample_rate;

	swr_alloc_set_opts2(&swr, &outLayout, outFormat, outSampleRate,
			    &codecCtx->ch_layout, codecCtx->sample_fmt,
			    codecCtx->sample_rate, 0, nullptr);
	if (!swr || swr_init(swr) < 0) {
		swr_free(&swr);
		avcodec_free_context(&codecCtx);
		avformat_close_input(&fmtCtx);
		return false;
	}

	_samples.sample_rate = outSampleRate;
	_samples.channels = 2;
	_samples.total_frames = 0;
	_samples.data.clear();

	AVPacket *pkt = av_packet_alloc();
	AVFrame *frame = av_frame_alloc();

	while (av_read_frame(fmtCtx, pkt) >= 0) {
		if (pkt->stream_index == audioStreamIndex) {
			if (avcodec_send_packet(codecCtx, pkt) == 0) {
				while (avcodec_receive_frame(codecCtx, frame) == 0) {
					int dstSamples = frame->nb_samples;
					std::vector<float> outBuf(dstSamples * 2);
					uint8_t *outPtr = reinterpret_cast<uint8_t *>(outBuf.data());
					int converted = swr_convert(swr, &outPtr,
								    dstSamples,
								    (const uint8_t **)frame->data,
								    frame->nb_samples);
					if (converted > 0) {
						_samples.data.insert(_samples.data.end(),
								     outBuf.begin(),
								     outBuf.begin() + converted * 2);
						_samples.total_frames += converted;
					}
				}
			}
		}
		av_packet_unref(pkt);
	}

	av_frame_free(&frame);
	av_packet_free(&pkt);
	swr_free(&swr);
	avcodec_free_context(&codecCtx);
	avformat_close_input(&fmtCtx);

	_loaded = (_samples.total_frames > 0);
	return _loaded;
#else
	blog(LOG_WARNING, "[obs-soundboard-censor] Audio decoding not supported on this platform");
	return false;
#endif
}

void AudioDecoder::unload()
{
	_loaded = false;
	_samples.data.clear();
	_samples.total_frames = 0;
}
