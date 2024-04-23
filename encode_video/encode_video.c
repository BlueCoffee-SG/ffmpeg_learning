#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>

// static 外部不能访问
static int encode(AVCodecContext *ctx,AVFrame *frame,AVPacket *pkt,FILE *out){
    int ret=-1;
    ret=avcodec_send_frame(ctx,frame);
    if(ret<0){
        av_log(NULL,AV_LOG_ERROR,"Failed to send frame to encoder!\n");
        goto _END;
    }

    while(ret>=0){
        //从编码器获取编码好的视频帧
        //第二个参数是编码好的视频帧保存路径
        ret=avcodec_receive_packet(ctx,pkt);
        if(ret==AVERROR(EAGAIN) || ret==AVERROR_EOF){
            return 0;
        }else if(ret<0){
            return -1; //退出tkyc
        } 
        /**
         * @brief 
         *  fwrite 参数：
         *   1 那些数据写到文件中
         *   2 每次写的是大小
         *  3、编码后的大的包的数节数
         *   4. 输出到那
         */
        fwrite(pkt->data,1,pkt->size,out);

        //对进行pkt计数 
        av_packet_unref(pkt);
    }
_END:
    return 0; 
}

int main(int argc,char* argv[]){

    int ret=-1;
    FILE *f=NULL;

    char *dst=NULL;
    char *codeName=NULL;
    const AVCodec *codec=NULL;
    AVCodecContext *ctx=NULL;
    AVFrame *frame =NULL;
    AVPacket *pkt=NULL;

    //1 输入参数
    if(argc<3){
        av_log(NULL,AV_LOG_ERROR,"arguments must be more 3\n");
        goto  _ERROR;
    }

    dst=argv[1];
    codeName=argv[2];


     //2.查找编码器
     codec=avcodec_find_encoder_by_name(codeName);
     if(!codec){
        av_log(NULL,AV_LOG_ERROR,"don't find Codec :%s",codeName);
        goto _ERROR;
     }

    //3. 创建编码器上下文
    ctx=avcodec_alloc_context3(codec);
    if(!ctx){
        av_log(NULL,AV_LOG_ERROR,"NO Memory \n");
        goto _ERROR;
    }

    // 4. 设置编码器参数
    ctx->width=640;
    ctx->height=480;
    //设置编码器码率
    ctx->bit_rate=50000;
    //设置时间基
    ctx->time_base=(AVRational){1,25};
      //设置帧率 25
     ctx->framerate=(AVRational){25,1};
    //一组帧大小 10为一组
    ctx->gop_size=10;
    //最大B帧，一般不超过3帧
    ctx->max_b_frames=1;
    // 视频源格式
    ctx->pix_fmt=AV_PIX_FMT_YUV411P;

    if(codec->id==AV_CODEC_ID_H264){
        //当编码器是h264,则可以设置私有参数
        av_opt_set(ctx->priv_data,"preset","slow",0);
    }


    // 5,把编码器与编码器上下文进行绑定
    /**
     * @brief avcodec_open2(AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options);
     *  1. avcodec 上下文
     *  2.codec
     * 3. options
     */
    ret=avcodec_open2(ctx,codec,NULL);  
    if(ret<0){
        av_log(ctx,AV_LOG_ERROR,"Don't open codec: %s \n",av_err2str(ret));
        goto _ERROR;
    }

    // 6. 创建输出文件
    f=fopen(dst,"wb");
    if(!f){
        av_log(NULL,AV_LOG_ERROR,"Don't open file: %s",dst);
        goto _ERROR;
    }

    // 7. 创建avframe
    frame=av_frame_alloc();
    if(!frame){
        av_log(NULL,AV_LOG_ERROR,"NO Momory!\n");
        goto _ERROR;
    }

    frame->width=ctx->width;
    frame->height=ctx->height;
    frame->format=ctx->pix_fmt;

    //对 av_frame data进行分配空间
    ret=av_frame_get_buffer(frame,0);
    if(ret<0){
        av_log(NULL,AV_LOG_ERROR,"cloud not allocate the video frame!\n");
        goto _ERROR;
    }

    // 8. 创建AVPacket
    pkt=av_packet_alloc();
    if(!pkt){
        av_log(NULL,AV_LOG_ERROR,"NO Momory!\n");
        goto _ERROR;
    }

    //9. 生成视频数据
    //视频数据要么从camera或者屏幕上
    for(int i=0;i<25;i++){
        //确保frame的data域是可以用的
        //需要讲frame给编码器。编码器拿到frame会锁定framedata.那这些数据进行编码
        //av_frame_make_writable 会检查frame的data域，看没写没有，锁定重新分配
        ret=av_frame_make_writable(frame);
        if(ret<0){
            break;
        }
        // y分量
        for(int y=0;y<ctx->height;y++){
            for(int x=0;x<ctx->width;x++){
                //操作的是yua中的y数据
                //yua是分层的，y是链路，uv控制颜色
                // data[0]表示的就是y数据
                //linesize[0]表示yuv数据中行大小
                //x代表横行移动,从0到宽度
                // i是帧有个数
                frame->data[0][y*frame->linesize[0]+x]=x+y+i*3; 

            }
        }

         // uv分量
        for(int y=0;y<ctx->height;y++){
            for(int x=0;x<ctx->width;x++){
                // 128代表黑色
                //u分量
                frame->data[1][y*frame->linesize[1]+x]=128+y+i*2; 
                //v分量 64黑色
                 frame->data[2][y*frame->linesize[2]+x]=64+y+i*5; 
            }
        }
        // 给frame编号
        frame->pts=i;

            //10 编码
        ret=encode(ctx,frame,pkt,f);
        if(ret==-1){
            goto _ERROR;
        }
    }

 //10 编码
    //编码器在编码时,如果我们数据都传送完了，在编码器缓存器中还残存一些剩余数据.要强制把剩余数据输出
    encode(ctx,NULL,pkt,f);

_ERROR:
    if(ctx){
        avcodec_free_context(&ctx);
    }
    if(frame){
        av_frame_free(&frame);
    }
    if(pkt){
        av_packet_free(&pkt);
    }
    //dst
    if(f){
        fclose(f);
    }
    return 0;
}