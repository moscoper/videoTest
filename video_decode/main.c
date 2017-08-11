//
//  main.c
//  video_decode
//
//  Created by cuifei on 2017/6/18.
//  Copyright © 2017年 cuifei. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

#define INBUF_SIZE 4096

static void pgm_save(uint8_t *buf, int wrap, int xsize, int ysize, char *filename){
    
    FILE *f;
    int i;
    
    f = fopen(filename, "wb");
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    for (i = 0; i < ysize; i++) {
        fwrite(buf+i*wrap, 1, xsize, f);
        
    }
    fclose(f);
    
}


static void decode(AVCodecContext *dec_ctx,AVFrame *frame,AVPacket *pkt,const char *filename){
    char buf[1024];
    int ret;
    
    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        exit(1);
    }
    while (ret >=0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        }else if(ret < 0){
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }
        
        printf("save frame %3d\n",dec_ctx->frame_number);
        fflush(stdout);
        snprintf(buf, sizeof(buf), "%s-%d",filename,dec_ctx->frame_number);
        
        pgm_save(frame->data[0], frame->linesize[0], frame->width, frame->height, buf);
        
        
        
    }
    
    
}

int encode(AVCodecContext *pCodecCtx,AVPacket *pPkt,AVFrame *pFrame,int *got_packet){
    
    int ret;
    *got_packet = 0;
    ret = avcodec_send_frame(pCodecCtx,pFrame);
    
    if(ret <0 && ret != AVERROR_EOF) {
        return ret;
    }
    
    ret = avcodec_receive_packet(pCodecCtx,pPkt);
    
    if(ret < 0 && ret != AVERROR(EAGAIN)) {
        return ret;
    }
    
    if(ret >= 0) {
        *got_packet = 1;
    }
    
    return 0;
    
}

int main(int argc, const char * argv[]) {
    char filename[] = "/Users/edz/Documents/testvideo/test2.mov";
    char *imageFilePath;
    const AVCodec *codec;

    AVCodecContext *pCodecCtx = NULL;
    AVCodec *inputCodec;
    AVCodecContext *inputCodecContext;
    AVFormatContext *avFormatContext;
    AVStream *avStream;
    AVFormatContext *inputAVFormateContext;

    AVFrame *frame;

    
    int ret,got_picture;
    int videoIndex = -1;
    int i;
    AVPacket *pkt;
    AVPacket *inputPkt;
    AVFrame *inputFrame;
    
    AVCodec *outCodec;
    AVCodecContext * outCodecContext;
    struct SwsContext *img_convert_ctx;
    imageFilePath = "/Users/edz/Documents/testvideo/video_images/test";
    
    av_register_all();
    avformat_network_init();
    inputAVFormateContext = avformat_alloc_context();
    avFormatContext = avformat_alloc_context();
    
//    avStream = avformat_new_stream(avFormatContext, NULL);
    avformat_alloc_output_context2(&avFormatContext, NULL, NULL, filename);
    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    
    pCodecCtx = avcodec_alloc_context3(codec);
    
    
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    pCodecCtx->width = 848;
    pCodecCtx->height = 480;
    
    pCodecCtx->time_base.num=1;
    pCodecCtx->time_base.den= 30;
    pCodecCtx->bit_rate= 800000;
    
    pCodecCtx->gop_size= 300;
    pCodecCtx->codec_tag = 0;
    
    if(avFormatContext->oformat->flags & AVFMT_GLOBALHEADER) {
        pCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    
    pCodecCtx->qmin = 10;
    pCodecCtx->qmax = 51;
    
    pCodecCtx->max_b_frames = 3;
    
    AVDictionary *dicParams = NULL;
    av_dict_set(&dicParams, "preset", "ultrafast", 0);
    av_dict_set(&dicParams, "tune", "zerolatency", 0);
    
//    avcodec_get_context_defaults3(avStream->codec, codec);
//    
//    avStream->codec->bit_rate = 400000;
//    avStream->codec->width = 848;
//    avStream->codec->height = 480;
//    avStream->codec->time_base.den = 25;
//    avStream->codec->time_base.num = 1;
//    avStream->codec->pix_fmt = AV_PIX_FMT_YUV420P;
//    avStream->codec->codec_tag = 0;
   
    if(avcodec_open2(pCodecCtx, codec, &dicParams)< 0) {
        printf("avcodec open error");
        exit(1);
    }
    
    avStream = avformat_new_stream(avFormatContext, codec);
    
    if(!avStream) {
        printf("Failed allocation output stream\n");
        exit(1);
    }
    
    avStream->time_base.num = 1;
    avStream->time_base.den = 30;
    avStream->codecpar->codec_tag = 0;
    
    avcodec_parameters_from_context(avStream->codecpar, pCodecCtx);
    
    
    if (!(avFormatContext->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&avFormatContext->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            printf("Could not open output file '%s'", filename);
            exit(1);
        }
    }
        
        
    if (avformat_write_header(avFormatContext, NULL)<0) {
        printf("Error occurred when opening output file\n");
        exit(1);
    }
    pkt = av_packet_alloc();
        if (!pkt) {
            exit(1);
        }
    char imageFilename[150];
    frame = av_frame_alloc();
    int y_length = 848*480;
    int uv_length = y_length/4;
    int j,k ;
    uint8_t *out_buffer = (uint8_t *) av_malloc(av_image_get_buffer_size(pCodecCtx->pix_fmt, 848, 480, 1));
    
    av_image_fill_arrays(frame->data, frame->linesize, out_buffer, pCodecCtx->pix_fmt, 848, 480, 1);
    
    inputFrame = av_frame_alloc();
    for (i = 1; i <= 848; i++) {
        sprintf(imageFilename, "%s%d.jpg",imageFilePath,i);
      
        inputAVFormateContext = avformat_alloc_context();
        ret= avformat_open_input(&inputAVFormateContext, imageFilename, NULL, NULL);
        printf("i= %d \n",i);
        if (ret != 0) {
            printf("Couldn't open input stream.  %d  \n",ret);
            exit(11);
        }
        
        ret = avformat_find_stream_info(inputAVFormateContext, NULL);
            if (ret < 0) {
                printf("Couldn't find stream information.\n");
                exit(1);
            }
            for (k = 0; k< inputAVFormateContext->nb_streams; k++) {
                if (inputAVFormateContext->streams[k]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
                    videoIndex = k;
                    break;
                }
            }
        
            if(videoIndex==-1){
                printf("Didn't find a video stream.\n");
                return -1;
            }
        
        
        inputCodecContext = inputAVFormateContext->streams[videoIndex]->codec;
        
            inputCodec = avcodec_find_decoder(inputCodecContext->codec_id);
        
        avcodec_open2(inputCodecContext, inputCodec, NULL);
        inputPkt = av_packet_alloc();
        
        img_convert_ctx = sws_getContext(inputCodecContext->width, inputCodecContext->height, inputCodecContext->pix_fmt,
                                         inputCodecContext->width, inputCodecContext->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
        while (av_read_frame(inputAVFormateContext, inputPkt) == 0) {
            if (inputPkt->stream_index == videoIndex) {
                avcodec_decode_video2(inputCodecContext, inputFrame, &got_picture,inputPkt);
                if (got_picture) {
                    uint8_t *data =  inputPkt->data;
                    
                    sws_scale(img_convert_ctx, (const uint8_t* const*)inputFrame->data, inputFrame->linesize, 0, pCodecCtx->height, frame->data, frame->linesize);
//                    memcpy(frame->data[0], data, y_length);
//                    for (j = 0; j < uv_length; j++) {
//                        *(frame->data[2]+j) = *(data+y_length+2 * j);
//                        *(frame->data[1]+j) = *(data+y_length+2 * j+1);
//                    }
                    
                    frame->format = pCodecCtx->pix_fmt;
                    frame->width = 848;
                    frame->height = 480;
                    frame->pts = (1.0 / 30) * 90 * i;
            
                    pkt->size = 0;
                    pkt->data = NULL;
                    av_init_packet(pkt);
                    
                    ret = encode(pCodecCtx,pkt,frame,&got_picture);
                    
                    if (got_picture) {
                        pkt->stream_index = avStream->index;
                        av_packet_rescale_ts(pkt,pCodecCtx->time_base,avStream->time_base);
                    }

                }
            }
        }
        
    
        
        //=========
//        FILE *f = fopen(imageFilename, "rb");
//        
//        uint8_t data[1024*1024];
//       
//        pkt->size = fread(data, 1,sizeof(data), f);
        
        
//        if (pkt->size <1) {
//            printf("read file error");
//            exit(2);
//        }
        
//        if (i == 848) {
            AVRational time_base = avFormatContext->streams[0]->time_base;
            AVRational r_frame_rate1 = {60,2};
            AVRational time_base_q = {1,AV_TIME_BASE};
            int64_t calc_duration = (double)AV_TIME_BASE * (1/av_q2d(r_frame_rate1));
            pkt->pts = av_rescale_q(i * calc_duration,time_base_q,time_base);
            pkt->dts = pkt->pts;
            pkt->pos = -1;
           
            avFormatContext->duration = pkt->duration * i;
//        }
        
        
        if (av_interleaved_write_frame(avFormatContext, pkt) < 0) {
            printf("Error muxing packet\n");
            exit(1);
        }
        
    }
    
    
    
    av_free_packet(pkt);
    av_write_trailer(avFormatContext);
    
    if (avFormatContext && !(avFormatContext->oformat->flags & AVFMT_NOFILE)) {
        avio_close(avFormatContext->pb);
        
    }
    avformat_free_context(avFormatContext);
    

    
   /**
    拆视频
    */
   /**ret = avformat_open_input(&avFormatContext, filename, NULL , NULL);
    if (ret != 0) {
        printf("Couldn't open input stream.\n");
        exit(1);
    }
    
    ret = avformat_find_stream_info(avFormatContext, NULL);
    if (ret < 0) {
        printf("Couldn't find stream information.\n");
        exit(1);
    }
    for (i = 0; i< avFormatContext->nb_streams; i++) {
        if (avFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
            break;
        }
    }
    
    if(videoIndex==-1){
        printf("Didn't find a video stream.\n");
        return -1;
    }
    
    AVCodecContext *c = avFormatContext->streams[videoIndex]->codec;
    
    codec = avcodec_find_decoder(c->codec_id);
    
    outCodec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if (!outCodec)
    {
        printf("Call avcodec_find_encoder function failed!\n");
        return 0;
    }
    
    outCodecContext = avcodec_alloc_context3(outCodec);
    if (!outCodecContext)
    {
        printf("Call avcodec_alloc_context3 function failed!\n");
        return 0;
    }
    
    outCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
    outCodecContext->pix_fmt = AV_PIX_FMT_YUVJ420P;
    
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
        printf("Could not open codec.\n");
        exit(1);
    }
    
    pkt = av_packet_alloc();
    if (!pkt) {
        exit(1);
    }
    AVPacket outPkt;
    
    av_init_packet(&outPkt);
//    if (!pkt) {
//        exit(1);
//    }

    frame = av_frame_alloc();
    
    int n = 0;
    
    outCodecContext->width = avFormatContext->streams[videoIndex]->codec->width;
    outCodecContext->height = avFormatContext->streams[videoIndex]->codec->height;
    
    outCodecContext->time_base.num = c->time_base.num;
    outCodecContext->time_base.den = c->time_base.den;
    
    ret = avcodec_open2(outCodecContext, outCodec, NULL);
    if (ret < 0) {
        printf("Could not open outcodec.\n");
        exit(1);
    }
    
    char dstFileName[150] ={0};
    char* outfilename ="/Users/edz/Documents/testvideo//video_images/test";
    
    while (av_read_frame(avFormatContext, pkt) ==0) {
        if (pkt->stream_index == videoIndex ) {
            ret = avcodec_decode_video2(c, frame, &got_picture, pkt);
    
            if(ret < 0){
                printf("Decode Error.\n");
                return -1;
            }
            
            if (got_picture) {
                
                  n++;
                av_new_packet(&outPkt, outCodecContext->width * outCodecContext->height *3);
                ret = avcodec_encode_video2(outCodecContext, &outPkt, frame, &got_picture);
                
                if(ret < 0)
                {
                    printf("Encode Error.\n");
                    avcodec_close(outCodecContext);
                    av_free(outCodecContext);
                    av_free_packet(&outPkt);
                    exit(1);
                }
                
                if (got_picture) {
                
                    
                    sprintf(dstFileName,"%s%d.jpg",outfilename,n);
                    printf("dstFileName=%s\n",dstFileName);
                        FILE *f = fopen(dstFileName, "wb");
                    printf("outPkt.size=%s\n",dstFileName);

                        fwrite(outPkt.data, sizeof(uint8_t), outPkt.size, f);
                        
                        fclose(f);
                        av_free_packet(&outPkt);
                
                   
                }
//                n++;
//                                char buf[1024];
//                sprintf(buf, "%s%d.jpg",outfilename,n);
//                    pgm_save(frame->data[0], frame->linesize[0], frame->width, frame->height, buf);
//                    FILE *f = fopen(buf, "wb");
//                    fwrite(frame->data[0], c->width*c->height*3, 1, f);
//
//                    fwrite(pkt->data, sizeof(uint8_t), pkt->size, f);
//                    
//                     fclose(f);
                
               
            }
        }
    
    }*/
    
    
    return 0;
}


















































