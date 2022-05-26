# syntax = docker/dockerfile:1.2

ARG TARGET_ARCH=amd64
ARG BAZEL_VERSION=5.0.0

FROM ubuntu:20.04 AS base
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y libasound2    

FROM base AS builddep
ARG TARGET_ARCH
ARG BAZEL_VERSION

RUN apt-get update && apt-get install -y \
    git \
    wget \
    unzip \
    build-essential \
    libasound2-dev

RUN if [ "$TARGET_ARCH" = "amd64" ]; then \
        wget https://github.com/bazelbuild/bazel/releases/download/${BAZEL_VERSION}/bazel-${BAZEL_VERSION}-installer-linux-x86_64.sh && \
        chmod +x bazel-${BAZEL_VERSION}-installer-linux-x86_64.sh && \
        ./bazel-${BAZEL_VERSION}-installer-linux-x86_64.sh --user && \
        echo "PATH=/root/bin:$PATH\n" >> /root/.bashrc && \
        echo "source /root/.bazel/bin/bazel-complete.bash" >> /root/.bashrc && \
        rm ./bazel-${BAZEL_VERSION}-installer-linux-x86_64.sh; \
    elif [ "$TARGET_ARCH" = "arm64" ]; then \
        apt-get update && apt-get install -y --no-install-recommends openjdk-11-jdk-headless && \
        wget https://urm.nvidia.com/artifactory/sw-ai-app-bazel/sw-ai-app-jarvis/tools_aarch64/bazel_aarch64_${BAZEL_VERSION} && \
        chmod +x bazel_aarch64_${BAZEL_VERSION} && \
        mv bazel_aarch64_${BAZEL_VERSION} /usr/local/bin/bazel; \
    fi

ENV PATH="/root/bin:${PATH}"

FROM builddep as builder

WORKDIR /work
COPY .bazelrc .gitignore WORKSPACE ./
COPY .git /work/.git
COPY scripts /work/scripts
COPY third_party /work/third_party
COPY riva /work/riva
ARG BAZEL_CACHE_ARG=""
RUN if [ "$TARGET_ARCH" = "amd64" ]; then \
       bazel test $BAZEL_CACHE_ARG //riva/clients/... --test_summary=detailed --test_output=all && \
       bazel build --stamp --config=release $BAZEL_CACHE_ARG //...; \
    elif [ "$TARGET_ARCH" = "arm64" ]; then \
       bazel test $BAZEL_CACHE_ARG //riva/clients/... --test_summary=detailed --test_output=all --define aarch64_build=1 && \
       bazel build --stamp --config=aarch64 --action_env BUILD_AARCH64="1" --define aarch64_build=1 $BAZEL_CACHE_ARG //...; \
    fi && \
    cp -R /work/bazel-bin/riva /opt

RUN ls -lah /work; ls -lah /work/.git; cat /work/.bazelrc

FROM base as riva-clients

WORKDIR /work
COPY --from=builder /opt/riva/clients/asr/riva_asr_client /usr/local/bin/ 
COPY --from=builder /opt/riva/clients/asr/riva_streaming_asr_client /usr/local/bin/ 
COPY --from=builder /opt/riva/clients/tts/riva_tts_client /usr/local/bin/ 
COPY --from=builder /opt/riva/clients/tts/riva_tts_perf_client /usr/local/bin/ 
COPY --from=builder /opt/riva/clients/nlp/riva_nlp_classify_tokens /usr/local/bin/ 
COPY --from=builder /opt/riva/clients/nlp/riva_nlp_punct /usr/local/bin/ 
COPY --from=builder /opt/riva/clients/nlp/riva_nlp_qa /usr/local/bin/
COPY examples /work/examples