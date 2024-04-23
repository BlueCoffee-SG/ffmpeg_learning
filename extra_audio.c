#include <stdio.h>
#include <libavutil/log.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>

int main(int argc, char *argv[]){
    //1. 处理一些参数
    char* src;
    char* dst;

    int ret,index=-1;

    AVFormatContext *pFormatCtx=NULL;
    AVFormatContext   *oFormatCtx=NULL;   
    const AVOutputFormat  *outFmt =NULL;
    AVStream *outStream=NULL;
    AVStream *inStream=NULL;
    AVPacket pkt;

    av_log_set_level(AV_LOG_DEBUG);
    if(argc<3) { //argv[0] extra_audio
        av_log(NULL,AV_LOG_INFO,"argument must be more than 3!");
        exit(-1);
    }

    // printf("com1=%s\n",*argv[0]);
    // printf("com2=%s\n",*argv[1]);
    // printf("com3=%s\n",*argv[2]);
    src=argv[1];
    dst=argv[2];


    //2.打开多模体函数
    /**
     * @brief 
     *  avformat_open_input:
     *      函数调用成功后，就会开辟一个apformat分配一个空间
     *      url: local file or http address
     *      acinputformat: 多媒体文件是mfv,但实现是mp4,如果不指定就会报错
     */
    if((ret=avformat_open_input(&pFormatCtx,src,NULL,NULL))<0){
        av_log(NULL,AV_LOG_ERROR,"%s\n",av_err2str(ret));
        exit(-1);
    }


    //3.从多媒体文件中找到音流
    /**
     *  返回 流的index
     * @brief 
     * enum AVMediaType type,
                        int wanted_stream_nb,
                        int related_stream,
                        const struct AVCodec **decoder_ret,
                        int flags
     *   AVMEDIA_TYPE_ATTACHMENT : 音频流
     * related_stream:用户请求的流的number,在多媒体文件中多个音频流的都有自己的stream_number,
     *      -1是查询第一个符合类型的流的number
     *  related_stream: 它与ts中的programme相关的，hr64中音视频流有多个分片组成. 
     *        每个分片音视频流对应不同的节目,ts中也不同节目 ,defualt=-1
     * AVCodec **decoder_ret : 解码优先解码器
     *  
     */

    index=av_find_best_stream(pFormatCtx,AVMEDIA_TYPE_AUDIO, -1,-1,NULL,0);
    if(index<0){
          av_log(NULL,AV_LOG_ERROR,"Does not include audio stream!\n");
          goto _ERROR;
    }

    //4.打开目的文件的上下文
    oFormatCtx=avformat_alloc_context();
    if(!oFormatCtx){
        av_log(NULL,AV_LOG_ERROR,"No Memory!\n");
        goto _ERROR;
    }
    /**
     * @brief Construct a new av guess format object
     *  *av_guess_format(const char *short_name,
                                      const char *filename,
                                      const char *mime_type);
        short_name 非NULL,检查short_name是不是与我们注册的格式匹配
        *filename： 输出文件的名字
        mime_type： 
     */
    outFmt=av_guess_format(NULL,dst,NULL);
    oFormatCtx->oformat=outFmt;


    //5.为目的文件，创建一个新的音频流
    /**
     * @brief Construct a new avformat new stream object
     * AVFormatContext *s, const struct AVCodec *c 
     *  AVFormatContext : 输出的 oformatContext
     * AVCodec :  解码器
     */
    outStream =avformat_new_stream(oFormatCtx,NULL);   

    //6.设置输出音频参数
    inStream=pFormatCtx->streams[index];
    /**
     * @brief Construct a new avcodec parameters copy object
     *  VCodecParameters *dst, const AVCodecParameters *src
     * VCodecParameters :参数输出给谁
     * AVCodecParameters ：参数从那里来
     */
     avcodec_parameters_copy(outStream->codecpar,inStream->codecpar);
     // 设置为0 会根据多媒体文件来自动识别编码器
     outStream->codecpar->codec_tag=0;

    //把输出上下文与目标文件绑定
    /**
     * @brief 
     *  AVIOContext **s, const char *url, int flags,
               const AVIOInterruptCB *int_cb, AVDictionary **options
        AVIOContext: 
        url: 输出的文件
        flags 对文件那些操作，read,write
        AVIOInterruptCB:  回调函数
        AVDictionary: 私有协议的选项
     */
    ret=avio_open2(&oFormatCtx->pb,dst,AVIO_FLAG_WRITE,NULL,NULL);
    if(ret<0){
        av_log(oFormatCtx,AV_LOG_ERROR,"%s",av_err2str(ret));
        goto _ERROR;
    }
    //7.写多媒体文件头到目的文件中,  
    ret=avformat_write_header(oFormatCtx,NULL);
    if(ret<0){
        av_log(oFormatCtx,AV_LOG_ERROR,"%s",av_err2str(ret));
        goto _ERROR;
    }
    //8.从源多媒体文件中读到音频数据到目的文件中
    /**
     * @brief Construct a new while object
     * AVFormatContext *s, AVPacket *pkt
     * AVFormatContext : 从那里读取音频帧, 指输出上下文
     * AVPacket： 读取数据放在那
     */
    while(av_read_frame(pFormatCtx ,&pkt)>=0){
        if(pkt.stream_index==index){
            //把音频数据写到文件中
            //修改时间戳
            /**
             * @brief  av_rescale_q_rnd
             * nt64_t a, AVRational bq, AVRational cq,
                         enum AVRounding rnd
                a: 原始pts
                bq: 输入流的时间基
                cq：输出流的时间基
                rnd: 除不尽，给一个近似值
             */

            //  pkt.pts = av_rescale_q_rnd(pkt.pts, inStream->time_base,\
            //   outStream->time_base,\
            //   (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX) );

            pkt.pts=av_rescale_q_rnd(pkt.pts,inStream->time_base,outStream->time_base,(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
          
           
            pkt.dts=pkt.pts;
            //音频帧时长
            pkt.duration=av_rescale_q(pkt.duration,inStream->time_base,outStream->time_base);
            //抽取只用一路音频，输出也只有这一路音频
            pkt.stream_index=0;
            pkt. pos=-1;
            av_interleaved_write_frame(oFormatCtx,&pkt); 
            //释放包
            av_packet_unref(&pkt);
        }
    }
    //9.写多媒体文件尾到文件中
    av_write_trailer(oFormatCtx);

    //10.将申请的资源释放

_ERROR:
    if(pFormatCtx){
        avformat_close_input(&pFormatCtx);
        pFormatCtx=NULL;
    }
    // 判断pkt打开没有
    if(oFormatCtx->pb){
        avio_close(oFormatCtx->pb);
    }
     if(oFormatCtx){
        avformat_free_context(oFormatCtx);
        oFormatCtx=NULL;
    }
    printf("program is finish~~~\n");
    return 0;
} 
