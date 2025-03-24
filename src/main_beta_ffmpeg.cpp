#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <iostream>
#include <ffmpeg/avformat.h>
#include <ffmpeg/avcodec.h>
#include <ffmpeg/swscale.h>
#include <ffmpeg/avutil.h>


GLuint textureID;

void loadFrameToTexture(AVFrame *frame) {
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frame->width, frame->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame->data[0]);
    glGenerateMipmap(GL_TEXTURE_2D);
}

bool initFFmpeg(const char* videoPath, AVFormatContext* &formatCtx, AVCodecContext* &codecCtx, int &videoStreamIndex) {
    if (avformat_open_input(&formatCtx, videoPath, nullptr, nullptr) != 0) {
        std::cerr << "Could not open video file!" << std::endl;
        return false;
    }

    if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
        std::cerr << "Could not find stream information!" << std::endl;
        return false;
    }

    videoStreamIndex = -1;
    for (int i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex == -1) {
        std::cerr << "Could not find video stream!" << std::endl;
        return false;
    }

    AVCodecParameters* codecParams = formatCtx->streams[videoStreamIndex]->codecpar;
    AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    if (!codec) {
        std::cerr << "Unsupported codec!" << std::endl;
        return false;
    }

    codecCtx = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(codecCtx, codecParams) < 0) {
        std::cerr << "Failed to copy codec parameters!" << std::endl;
        return false;
    }

    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        std::cerr << "Could not open codec!" << std::endl;
        return false;
    }

    return true;
}

void renderLoop(GLFWwindow* window, AVFormatContext* formatCtx, AVCodecContext* codecCtx, int videoStreamIndex) {
    AVPacket packet;
    AVFrame* frame = av_frame_alloc();
    AVFrame* frameRGB = av_frame_alloc();
    int width = codecCtx->width;
    int height = codecCtx->height;
    
    struct SwsContext* swsCtx = sws_getContext(width, height, codecCtx->pix_fmt, width, height, AV_PIX_FMT_RGBA, SWS_BICUBIC, nullptr, nullptr, nullptr);

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, width, height, 1);
    uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(frameRGB->data, frameRGB->linesize, buffer, AV_PIX_FMT_RGBA, width, height, 1);

    while (!glfwWindowShouldClose(window)) {
        if (av_read_frame(formatCtx, &packet) < 0)
            break;

        if (packet.stream_index == videoStreamIndex) {
            if (avcodec_send_packet(codecCtx, &packet) < 0) {
                std::cerr << "Error sending packet for decoding!" << std::endl;
                continue;
            }

            if (avcodec_receive_frame(codecCtx, frame) < 0) {
                std::cerr << "Error receiving frame from decoder!" << std::endl;
                continue;
            }

            sws_scale(swsCtx, frame->data, frame->linesize, 0, height, frameRGB->data, frameRGB->linesize);
            loadFrameToTexture(frameRGB);

            glClear(GL_COLOR_BUFFER_BIT);
            glBindTexture(GL_TEXTURE_2D, textureID);

            // Render texture
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    av_frame_free(&frame);
    av_frame_free(&frameRGB);
    av_free(buffer);
    sws_freeContext(swsCtx);
}

int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed!" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Video", nullptr, nullptr);
    if (!window) {
        std::cerr << "GLFW window creation failed!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        return -1;
    }

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    AVFormatContext* formatCtx = nullptr;
    AVCodecContext* codecCtx = nullptr;
    int videoStreamIndex;

    if (!initFFmpeg("assets/IntrodusionToNextron.mp4", formatCtx, codecCtx, videoStreamIndex)) {
        std::cerr << "FFmpeg initialization failed!" << std::endl;
        return -1;
    }

    renderLoop(window, formatCtx, codecCtx, videoStreamIndex);

    avcodec_free_context(&codecCtx);
    avformat_close_input(&formatCtx);
    glfwTerminate();
    return 0;
}
