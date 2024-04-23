#include <stdio.h>
#include <libavutil/log.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>

int main(int argc, char *argv[]){
    //1. 处理一些参数
    char* src;
    char* dst;
    int *stream_map=NULL;
    int stream_idx=0;

    int ret,index=-1;

    AVFormatContext *pFormatCtx=NULL;
    AVFormatContext   *oFormatCtx=NULL;   
    const AVOutputFormat  *outFmt =NULL;
 
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


    //3.从多媒体文件中找到视频流
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

    // index=av_find_best_stream(pFormatCtx,AVMEDIA_TYPE_VIDEO, -1,-1,NULL,0);
    // if(index<0){
    //       av_log(NULL,AV_LOG_ERROR,"Does not include audio stream!\n");
    //       goto _ERROR;
    // }

    //4.打开目的文件的上下文
    /**
     * @brief Construct a new avformat alloc output context2 object
     *  avformat_alloc_output_context2(AVFormatContext **ctx, const   *oformat,
                                   const char *format_name, const char *filename)
            AVFormatContext 输出格式上下文 
            oformat: 通过gets获取到的,上下文使用的format,null会使用文件名与format_name
            format_name:null通过filename找到formatname
            filename: 目标文件
     */
    avformat_alloc_output_context2(&oFormatCtx,NULL,NULL,dst);
    //没有获取到ofmatctx
    if(!oFormatCtx){
        av_log(NULL,AV_LOG_ERROR,"NO MEMORY!\n");
        goto _ERROR;
    }


    // oFormatCtx=avformat_alloc_context();
    // if(!oFormatCtx){
    //     av_log(NULL,AV_LOG_ERROR,"No Memory!\n");
    //     goto _ERROR;
    // }
    /**
     * @brief Construct a new av guess format object
     *  *av_guess_format(const char *short_name,
                                      const char *filename,
                                      const char *mime_type);
        short_name 非NULL,检查short_name是不是与我们注册的格式匹配
        *filename： 输出文件的名字
        mime_type： 
     */
    // outFmt=av_guess_format(NULL,dst,NULL);
    // oFormatCtx->oformat=outFmt;


    //5.为目的文件，创建一个新的视频流
    //要为多媒体每个路流创建,创建一个stream
    //for会遍历多媒体文件的每个路流
    //在多媒体文件中不仅包含音频，视频流还有meta数据的流(它是没有义意)，要过滤
    stream_map=av_calloc(pFormatCtx->nb_streams,sizeof(int));
     if(!stream_map ){
        av_log(NULL,AV_LOG_ERROR,"NO MEMORY!\n");
        goto _ERROR;
    }
    for(int i=0;i<pFormatCtx->nb_streams;i++){
        AVStream  *outStream=NULL;
        AVStream *inStream=pFormatCtx->streams[i];
        AVCodecParameters *inCodecPar=inStream->codecpar;
        if(inCodecPar->codec_type!=AVMEDIA_TYPE_AUDIO&&
            inCodecPar->codec_type!=AVMEDIA_TYPE_VIDEO&&
            inCodecPar->codec_type!=AVMEDIA_TYPE_SUBTITLE){
                stream_map[i]=-1;
                continue;
            }
        stream_map[i]=stream_idx++;
        //5. 为目标的文件，创建一个新的视频流
        outStream =avformat_new_stream(oFormatCtx,NULL);
        if(!outStream){
            av_log(NULL,AV_LOG_ERROR,"NO MEMORY!\n");
            goto _ERROR;
        }
        avcodec_parameters_copy(outStream->codecpar,inStream->codecpar);
        // 设置为0 会根据多媒体文件来自动识别编码器
        outStream->codecpar->codec_tag=0;
    }
  

  

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
        AVStream *inStream,*outStream;
        inStream=pFormatCtx->streams[pkt.stream_index];
        if(stream_map[pkt.stream_index]<0){
             av_packet_unref(&pkt);   
             continue; 
             
        }
        //原文件的streamindex对应目标文件的stream index
        pkt.stream_index=stream_map[pkt.stream_index];
        //输出流
        outStream=oFormatCtx->streams[pkt.stream_index];
        av_packet_rescale_ts(&pkt,inStream->time_base,outStream->time_base);
        pkt.pos=-1;
        av_interleaved_write_frame(oFormatCtx,&pkt);
        av_packet_unref(&pkt);
      
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
    if(stream_map){
        av_free(stream_map);
    }
    printf("program is finish~~~\n");
    return 0;
} 
