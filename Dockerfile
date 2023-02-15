# From https://github.com/krishnaji/az-aks-unreal-pixel-streaming/blob/UE4.27/Game/Dockerfile

# Perform the build in an Unreal Engine container image that includes the Engine Tools and Pixel Streaming for Linux
FROM --platform=${BUILDPLATFORM:-linux/amd64} ghcr.io/epicgames/unreal-engine:dev-5.0.3 AS builder

# Copy UE5 project (assumes `.uproject` in this directory)
COPY  --chown=ue4:ue4 . /tmp/project
WORKDIR  /tmp/project

# Package the example Unreal project
RUN /home/ue4/UnrealEngine/Engine/Build/BatchFiles/RunUAT.sh \
      BuildCookRun \
       -utf8output \
       -platform=Linux \
       -clientconfig=Shipping \
       -serverconfig=Shipping \
       -project=/tmp/project/TwinCity.uproject \
       -noP4 -nodebuginfo -allmaps \
       -cook -build -stage -prereqs -pak -archive \
       -archivedirectory=/tmp/project/dist

# Copy the packaged files into a container image that includes CUDA but doesn't include any Unreal Engine components
FROM --platform=${BUILDPLATFORM:-linux/amd64} ghcr.io/epicgames/unreal-engine:runtime-pixel-streaming
WORKDIR /home/ue4/project
COPY --from=builder --chown=ue4:ue4 /tmp/project/dist/Linux ./

# Establish ENV
ENV RES_X=1920 \
    RES_Y=1080 \
    SIGNAL_URL=ws://127.0.0.1:8888

# Start pixel streaming
CMD ["/bin/bash", "-c", ".TwinCity.sh -PixelStreamingURL=${SIGNAL_URL} -RenderOffscreen -Unattended -ForceRes -ResX=${RES_X} -ResY=${RES_Y} -AllowPixelStreamingCommands ${EXTRA_ARGS}" ]