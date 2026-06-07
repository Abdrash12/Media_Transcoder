#include "MP4ToMP3Converter.h"
#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/audio_fifo.h>
}

MP4ToMP3Converter::~MP4ToMP3Converter() { cleanup(); }

bool MP4ToMP3Converter::execute(const std::string& sourcePath, const std::string& destPath, std::atomic<float>& progress) {
    if (!openInput(sourcePath)) { std::cerr << "\n[Fatal] Demuxer failed.\n"; cleanup(); return false; }
    if (!initDecoder()) { std::cerr << "\n[Fatal] Decoder failed.\n"; cleanup(); return false; }
    if (!initEncoder(destPath)) { std::cerr << "\n[Fatal] Encoder failed.\n"; cleanup(); return false; }
    if (!initResampler()) { std::cerr << "\n[Fatal] Resampler failed.\n"; cleanup(); return false; }
    if (!initFifo()) { std::cerr << "\n[Fatal] Audio Buffer failed.\n"; cleanup(); return false; }

    bool success = processStream(progress);
    cleanup();
    
    return success;
}

bool MP4ToMP3Converter::openInput(const std::string& sourcePath) {
    inputFormatCtx = avformat_alloc_context();
    if (avformat_open_input(&inputFormatCtx, sourcePath.c_str(), nullptr, nullptr) != 0) return false;
    if (avformat_find_stream_info(inputFormatCtx, nullptr) < 0) return false;

    audioStreamIndex = -1;
    for (unsigned int i = 0; i < inputFormatCtx->nb_streams; i++) {
        if (inputFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
            break;
        }
    }
    if (audioStreamIndex == -1) return false;
    totalDuration = inputFormatCtx->duration;
    return true;
}

bool MP4ToMP3Converter::initDecoder() {
    AVCodecParameters* codecParams = inputFormatCtx->streams[audioStreamIndex]->codecpar;
    const AVCodec* decoder = avcodec_find_decoder(codecParams->codec_id);
    if (!decoder) return false;

    decoderCtx = avcodec_alloc_context3(decoder);
    if (avcodec_parameters_to_context(decoderCtx, codecParams) < 0) return false;
    if (avcodec_open2(decoderCtx, decoder, nullptr) < 0) return false;
    return true;
}

bool MP4ToMP3Converter::initEncoder(const std::string& destPath) {
    if (avformat_alloc_output_context2(&outputFormatCtx, nullptr, "mp3", destPath.c_str()) < 0) return false;

    const AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_MP3);
    if (!encoder) return false;

    encoderCtx = avcodec_alloc_context3(encoder);
    encoderCtx->sample_rate = 44100; 
    encoderCtx->bit_rate = 192000;   
    encoderCtx->time_base = {1, 44100}; // Force exact timing for LAME
    av_channel_layout_default(&encoderCtx->ch_layout, 2);
    encoderCtx->sample_fmt = AV_SAMPLE_FMT_FLTP; 

    if (avcodec_open2(encoderCtx, encoder, nullptr) < 0) return false;

    AVStream* outStream = avformat_new_stream(outputFormatCtx, nullptr);
    if (!outStream) return false;
    if (avcodec_parameters_from_context(outStream->codecpar, encoderCtx) < 0) return false;

    if (!(outputFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&outputFormatCtx->pb, destPath.c_str(), AVIO_FLAG_WRITE) < 0) return false;
    }
    if (avformat_write_header(outputFormatCtx, nullptr) < 0) return false;
    return true;
}

bool MP4ToMP3Converter::initResampler() {
    if (swr_alloc_set_opts2(&resampleCtx,
                            &encoderCtx->ch_layout, encoderCtx->sample_fmt, encoderCtx->sample_rate,
                            &decoderCtx->ch_layout, decoderCtx->sample_fmt, decoderCtx->sample_rate,
                            0, nullptr) < 0) return false;
    if (swr_init(resampleCtx) < 0) return false;
    return true;
}

bool MP4ToMP3Converter::initFifo() {
    audioFifo = av_audio_fifo_alloc(encoderCtx->sample_fmt, encoderCtx->ch_layout.nb_channels, 1);
    if (!audioFifo) return false;
    return true;
}

bool MP4ToMP3Converter::processStream(std::atomic<float>& progress) {
    AVPacket* inPacket = av_packet_alloc();
    AVPacket* outPacket = av_packet_alloc();
    AVFrame* decodedFrame = av_frame_alloc();
    globalPts = 0;

    while (av_read_frame(inputFormatCtx, inPacket) >= 0) {
        if (inPacket->stream_index == audioStreamIndex) {
            if (avcodec_send_packet(decoderCtx, inPacket) == 0) {
                while (avcodec_receive_frame(decoderCtx, decodedFrame) == 0) {
                    
                    // 1. SAFELY create a fresh resampled frame on every iteration
                    int outSamples = swr_get_out_samples(resampleCtx, decodedFrame->nb_samples);
                    AVFrame* resampledFrame = av_frame_alloc();
                    resampledFrame->sample_rate = encoderCtx->sample_rate;
                    resampledFrame->format = encoderCtx->sample_fmt;
                    av_channel_layout_copy(&resampledFrame->ch_layout, &encoderCtx->ch_layout);
                    resampledFrame->nb_samples = outSamples;
                    av_frame_get_buffer(resampledFrame, 0);

                    // Normalize the audio math
                    int actualOutSamples = swr_convert(resampleCtx, resampledFrame->data, outSamples, 
                                                       (const uint8_t**)decodedFrame->data, decodedFrame->nb_samples);

                    // 2. Dump the audio into the holding tank (FIFO)
                    if (actualOutSamples > 0) {
                        av_audio_fifo_realloc(audioFifo, av_audio_fifo_size(audioFifo) + actualOutSamples);
                        av_audio_fifo_write(audioFifo, (void**)resampledFrame->data, actualOutSamples);
                    }
                    
                    // Safely destroy the frame completely to prevent memory leaks and reset bugs!
                    av_frame_free(&resampledFrame);

                    // 3. Siphon EXACTLY 1152 samples out of the tank to feed the MP3 Encoder
                    while (av_audio_fifo_size(audioFifo) >= encoderCtx->frame_size) {
                        AVFrame* encodeFrame = av_frame_alloc();
                        encodeFrame->nb_samples = encoderCtx->frame_size;
                        av_channel_layout_copy(&encodeFrame->ch_layout, &encoderCtx->ch_layout);
                        encodeFrame->format = encoderCtx->sample_fmt;
                        encodeFrame->sample_rate = encoderCtx->sample_rate;
                        av_frame_get_buffer(encodeFrame, 0);

                        av_audio_fifo_read(audioFifo, (void**)encodeFrame->data, encoderCtx->frame_size);
                        
                        // Sync the timing
                        encodeFrame->pts = globalPts;
                        globalPts += encodeFrame->nb_samples;

                        // 4. Encode and Write
                        if (avcodec_send_frame(encoderCtx, encodeFrame) == 0) {
                            while (avcodec_receive_packet(encoderCtx, outPacket) == 0) {
                                av_packet_rescale_ts(outPacket, encoderCtx->time_base, outputFormatCtx->streams[0]->time_base);
                                outPacket->stream_index = 0;
                                av_interleaved_write_frame(outputFormatCtx, outPacket);
                                av_packet_unref(outPacket);
                            }
                        }
                        av_frame_free(&encodeFrame);
                    }

                    // Progress UI math
                    if (totalDuration > 0 && inPacket->pts != AV_NOPTS_VALUE) {
                        int64_t currentPtsTime = av_rescale_q(inPacket->pts, inputFormatCtx->streams[audioStreamIndex]->time_base, {1, AV_TIME_BASE});
                        float currentProg = static_cast<float>(currentPtsTime) / static_cast<float>(totalDuration);
                        progress = (currentProg > 1.0f) ? 1.0f : (currentProg < 0.0f ? 0.0f : currentProg);
                    }
                    av_frame_unref(decodedFrame);
                }
            }
        }
        av_packet_unref(inPacket); // Ensure video packets don't leak memory
    }

    // 5. Flush any remaining audio left in the tank at the very end
    while (av_audio_fifo_size(audioFifo) > 0) {
        int frame_size = FFMIN(av_audio_fifo_size(audioFifo), encoderCtx->frame_size);
        AVFrame* encodeFrame = av_frame_alloc();
        encodeFrame->nb_samples = frame_size;
        av_channel_layout_copy(&encodeFrame->ch_layout, &encoderCtx->ch_layout);
        encodeFrame->format = encoderCtx->sample_fmt;
        encodeFrame->sample_rate = encoderCtx->sample_rate;
        av_frame_get_buffer(encodeFrame, 0);

        av_audio_fifo_read(audioFifo, (void**)encodeFrame->data, frame_size);
        encodeFrame->pts = globalPts;
        globalPts += encodeFrame->nb_samples;

        if (avcodec_send_frame(encoderCtx, encodeFrame) == 0) {
            while (avcodec_receive_packet(encoderCtx, outPacket) == 0) {
                av_packet_rescale_ts(outPacket, encoderCtx->time_base, outputFormatCtx->streams[0]->time_base);
                outPacket->stream_index = 0;
                av_interleaved_write_frame(outputFormatCtx, outPacket);
                av_packet_unref(outPacket);
            }
        }
        av_frame_free(&encodeFrame);
    }

    // Flush Encoder to write the final bytes
    avcodec_send_frame(encoderCtx, nullptr);
    while (avcodec_receive_packet(encoderCtx, outPacket) == 0) {
        av_packet_rescale_ts(outPacket, encoderCtx->time_base, outputFormatCtx->streams[0]->time_base);
        outPacket->stream_index = 0;
        av_interleaved_write_frame(outputFormatCtx, outPacket);
        av_packet_unref(outPacket);
    }
    av_write_trailer(outputFormatCtx);

    av_packet_free(&inPacket);
    av_packet_free(&outPacket);
    av_frame_free(&decodedFrame);

    progress = 1.0f;
    return true;
}

void MP4ToMP3Converter::cleanup() {
    if (audioFifo) { av_audio_fifo_free(audioFifo); audioFifo = nullptr; }
    if (resampleCtx) swr_free(&resampleCtx);
    if (encoderCtx) { avcodec_free_context(&encoderCtx); encoderCtx = nullptr; }
    if (decoderCtx) { avcodec_free_context(&decoderCtx); decoderCtx = nullptr; }
    if (outputFormatCtx) {
        if (!(outputFormatCtx->oformat->flags & AVFMT_NOFILE)) avio_closep(&outputFormatCtx->pb);
        avformat_free_context(outputFormatCtx);
        outputFormatCtx = nullptr;
    }
    if (inputFormatCtx) { avformat_close_input(&inputFormatCtx); inputFormatCtx = nullptr; }
}