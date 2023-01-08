/*
 * SPDX-FileCopyrightText: Copyright (c) 2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <grpcpp/grpcpp.h>
#include <strings.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <mutex>
#include <numeric>
#include <queue>
#include <sstream>
#include <string>
#include <thread>

#include "riva/proto/riva_asr.grpc.pb.h"
#include "riva/proto/riva_nmt.grpc.pb.h"
#include "riva/utils/wav/wav_reader.h"
#include "riva_asr_client_helper.h"

using grpc::Status;
using grpc::StatusCode;

namespace nr = nvidia::riva;
namespace nr_asr = nvidia::riva::asr;
namespace nr_nmt = nvidia::riva::nmt;

struct Results {
  std::vector<std::string> final_transcripts;
  std::vector<float> final_scores;
  std::string partial_transcript;
  std::vector<std::vector<nr_asr::WordInfo>> final_time_stamps;
  std::vector<nr_asr::WordInfo>partial_time_stamps;
  int request_cnt;
  float audio_processed;
};

class S2SClientCall {

  public:
    S2SClientCall(uint32_t _corr_id) {}; //not sure this is needed
    nr_nmt::StreamingTranslateSpeechToSpeechResponse response;
    std::queue<nr_nmt::StreamingTranslateSpeechToSpeechRequest> requests;
    grpc::ClientContext s2s_context_;
    std::unique_ptr<grpc::ClientReaderWriterInterface<
                      nr_nmt::StreamingTranslateSpeechToSpeechRequest,
                      nr_nmt::StreamingTranslateSpeechToSpeechResponse>> streamer;

    grpc::Status finish_status;

};

class SpeechTranslateClientCall {

  public:
    SpeechTranslateClientCall(uint32_t _corr_id) {}; //not sure this is needed
    nr_nmt::StreamingTranslateSpeechToTextResponse response;
    std::queue<nr_nmt::StreamingTranslateSpeechToTextRequest> requests;
    grpc::ClientContext speech_translate_context_;
    std::unique_ptr<grpc::ClientReaderWriterInterface<
                      nr_nmt::StreamingTranslateSpeechToTextRequest,
                      nr_nmt::StreamingTranslateSpeechToTextResponse>> streamer;

    grpc::Status finish_status;

};

class ClientCall {
 public:
  ClientCall(uint32_t _corr_id, bool word_time_offsets);

  void AppendResult(const nr_asr::StreamingRecognitionResult& result);

  void PrintResult(bool audio_device, std::ofstream& output_file);

  // Container for the data we expect from the server.
  nr_nmt::StreamingTranslateSpeechToTextResponse response;
  std::queue<nr_nmt::StreamingTranslateSpeechToTextRequest> requests;

  // std::mutex request_mutex;
  // Context for the client. It could be used to convey extra information to
  // the server and/or tweak certain RPC behaviors.
  grpc::ClientContext context;
  std::unique_ptr<grpc::ClientReaderWriterInterface<
                    nr_nmt::StreamingTranslateSpeechToTextRequest, nr_nmt::StreamingTranslateSpeechToTextResponse>>
      streamer;

  std::unique_ptr<Stream> stream;
  std::chrono::time_point<std::chrono::steady_clock> send_time;

  uint32_t corr_id_;
  bool word_time_offsets_;

  Results latest_result_;

  std::vector<std::chrono::time_point<std::chrono::steady_clock>> send_times, recv_times;
  std::vector<bool> recv_final_flags;

  grpc::Status finish_status;

};  // ClientCall
