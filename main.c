#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "jpeglib.h"
#include <setjmp.h>

#include <math.h>

#define DEBUG

//自定义数学函数
double 
fmod_wrap(const double v, const double w)
{
    return v < 0. ? fmod(v, w) + w : fmod(v, w);
}

float 
fmodf_wrap(const float v, const float w)
{
    return v < 0. ? fmodf(v, w) + w : fmodf(v, w);
}

int sgn(const double d)
{
//    if(d<0) return -1;
//    else if (d==0) return 0;
//    else return 1;
    return (d>0)?(1):((d==0)?(0):(-1));
}

int sgnf(const float d)
{
//    if(d<0) return -1;
//    else if (d==0) return 0;
//    else return 1;
    return (d>0)?(1):((d==0)?(0):(-1));
}

double pow2(const double d)
{
    return d*d;
}

float pow2f(const float d)
{
    return d*d;
}



struct my_error_mgr {
    struct jpeg_error_mgr pub;    /* "public" fields */
    jmp_buf        setjmp_buffer;    /* for return to caller */
};

typedef struct my_error_mgr *my_error_ptr;

METHODDEF(void)my_error_exit(j_common_ptr cinfo)
{
    my_error_ptr    myerr = (my_error_ptr) cinfo->err;
    (*cinfo->err->output_message) (cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}





GLOBAL(void)
write_JPEG_file(char *filename, int quality, JSAMPLE * image_buffer, int image_width, int image_height)
{
    /* More stuff */
    FILE           *outfile;/* target file */
    JSAMPROW    row_pointer[1];    /* pointer to JSAMPLE row[s] */
    int        row_stride;    /* physical row width in image buffer */

    struct jpeg_compress_struct cinfo;

    //出错处理
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_compress(&cinfo);
    if ((outfile = fopen(filename, "wb")) == NULL) {
        fprintf(stderr, "can't open %s\n", filename);
        exit(1);
    }
    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = image_width;    /* image width and height, in
                         * pixels */
    cinfo.image_height = image_height;
    cinfo.input_components = 3;    /* # of color components per pixel */
    cinfo.in_color_space = JCS_RGB;    /* colorspace of input image */

    jpeg_set_defaults(&cinfo);

    jpeg_set_quality(&cinfo, quality, TRUE    /* limit to baseline-JPEG
                                  values */ );

    jpeg_start_compress(&cinfo, TRUE);

    row_stride = image_width * 3;    /* JSAMPLEs per row in image_buffer */

    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &image_buffer[cinfo.next_scanline * row_stride];
        (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    fclose(outfile);
    jpeg_destroy_compress(&cinfo);
}

int 
main()
{

#ifdef DEBUG
    //计时
    struct timeval    tpstart, tpend;
    float        timeuse;
#endif

    char           *filename = "a.jpg";

    FILE           *infile;    /* source file */
    JSAMPARRAY    buffer;    /* Output row buffer */
    int        row_stride;    /* physical row width in output buffer */

    //文件检查
    if ((infile = fopen(filename, "rb")) == NULL) {
        fprintf(stderr, "文件不存在： %s \n", filename);
        return 1;
    }
    struct jpeg_decompress_struct cinfo;

    //出错处理
    struct my_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return 2;
    }
    //准备
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    (void)jpeg_read_header(&cinfo, TRUE);
    (void)jpeg_start_decompress(&cinfo);

    row_stride = cinfo.output_width * cinfo.output_components;
    buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) & cinfo, JPOOL_IMAGE, row_stride, 1);

    //获取参数申请内存
    uint32_t        pano_width = cinfo.output_width;
    uint32_t        pano_height = cinfo.output_height;
    int        pano_components = cinfo.output_components;
    JSAMPLE        *pano_buffer = malloc(pano_width * pano_height * pano_components);
    JSAMPLE        *pano_buffer_p = pano_buffer;

    printf("Pano图片大小： %dpx x %dpx \n",pano_width,pano_height);

#ifdef DEBUG
    //计时开始
    gettimeofday(&tpstart, NULL);
#endif

    //开始读取
    while (cinfo.output_scanline < cinfo.output_height) {
        (void)jpeg_read_scanlines(&cinfo, buffer, 1);
        memcpy(pano_buffer_p, buffer[0], row_stride);
        pano_buffer_p += row_stride;
        //put_scanline_someplace(buffer[0], row_stride);
    }
    pano_buffer_p = 0;

#ifdef DEBUG
    //读取完毕计时
    gettimeofday(&tpend, NULL);
    timeuse = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
    timeuse /= 1000000;
    //输出耗时
    fprintf(stderr, "读取完毕计时: %fs\n", timeuse);
    //重新计时开始
    gettimeofday(&tpstart, NULL);
#endif

    //开始映射生成
    uint64_t    sky_w = sqrt(pano_width*pano_height/6.);
    uint64_t    sky_h = sky_w;
    int         sky_quality = 50;
    JSAMPLE     *sky_f = malloc(sky_w * sky_h * 3);
    JSAMPLE     *sky_b = malloc(sky_w * sky_h * 3);
    JSAMPLE     *sky_l = malloc(sky_w * sky_h * 3);
    JSAMPLE     *sky_r = malloc(sky_w * sky_h * 3);
    JSAMPLE     *sky_u = malloc(sky_w * sky_h * 3);
    JSAMPLE     *sky_d = malloc(sky_w * sky_h * 3);

    printf("sky图片大小： %lldpx x %lldpx , 质量：%d\n",sky_w,sky_h,sky_quality);

    int64_t     sx, sy, px, py;
    float       sxf, syf, pxf, pyf;

    float       tmp_a,tmp_b,tmp_c,tmp_d,tmp_e;

    for (sx = 0; sx < sky_w; sx++) {
        for (sy = 0; sy < sky_h; sy++) {

            sxf = (float)sx / (float)sky_w;
            syf = (float)sy / (float)sky_h;

            tmp_a=sgnf(1 - 2 * sxf);
            tmp_b=pow2f(2 * syf - 1);
            tmp_c=pow2f(2 * sxf - 1);
            tmp_d=atanf(1 / (1 - 2 * sxf));
            tmp_e=sqrtf(1 + tmp_b + tmp_c);

            //front
            //xx//pxf = (fmodf_wrap(atan2f(1 - 2 * sxf, -1) / (2 * (float)M_PI), 1)) ;
            pxf = fmodf_wrap((atanf((2 * sxf) - 1) + (2 * tmp_a * (float)M_PI_2)) / (2 * (float)M_PI), 1);
            px = pxf * pano_width;
            pyf = (fmodf_wrap(acosf((2 * syf - 1) / tmp_e) / (float)M_PI, 1)) ;
            py = pyf * pano_height;
            memcpy(sky_f + (
                       (sky_h - sy - 1)//sy
                       * sky_w * 3 +
                       (sky_w - sx - 1)//sx
                       * 3), pano_buffer + (
                       py//py
                       * pano_width * pano_components +
                       px//py
                       * pano_components), 3);

            //back
            //xx//pxf = (fmodf_wrap(atan2f(1 - 2 * sxf, 1) / (2 * (float)M_PI), 1)) ;
            pxf = fmodf_wrap(atanf(1 - 2 * sxf) / (2 * (float)M_PI), 1);
            px = pxf * pano_width;
            //pyf = (fmodf_wrap(acosf((2 * syf - 1) / tmp_e) / (float)M_PI, 1)) ;
            //py = pyf * pano_height;
            memcpy(sky_b + (
                       (sky_h - sy - 1)//sy
                       * sky_w * 3 +
                       sx//sx
                       * 3), pano_buffer + (
                       py//py
                       * pano_width * pano_components +
                       px//px
                       * pano_components), 3);

            //left
            //aa//pxf = (fmodf_wrap(atan2f(-1, 1 - 2 * sxf) / (2 * (float)M_PI), 1)) ;
            pxf = fmodf_wrap((((tmp_a - 1) * (float)M_PI_2) - tmp_d) / (2 * (float)M_PI),1);
            px = pxf * pano_width;
            pyf = (fmodf_wrap(acosf((2 * syf - 1) / tmp_e) / (float)M_PI, 1)) ;
            py = pyf * pano_height;
            memcpy(sky_l + (
                       (sky_h - sy - 1)//sy
                       * sky_w * 3 +
                       sx//sx
                       * 3), pano_buffer + (
                       py//py
                       * pano_width * pano_components +
                       px//px
                       * pano_components), 3);

            //right
            //xx//pxf = (fmodf_wrap(atan2f(1, 1 - 2 * sxf) / (2 * (float)M_PI), 1)) ;
            pxf = fmodf_wrap((((1 - tmp_a) * (float)M_PI_2) + tmp_d) / (2 * (float)M_PI), 1);
            //pxf = fmodf_wrap((atanf(1 / (((-2) * sxf) + 1)) + ((1 - sgnf(((-2) * sxf) + 1)) * (float)M_PI_2)) / ((2 * (float)M_PI)),1);
            px = pxf * pano_width;
            //pyf = (fmodf_wrap(acosf((2 * syf - 1) / tmp_e) / (float)M_PI, 1)) ;
            //py = pyf * pano_height;
            memcpy(sky_r + (
                       (sky_h - sy - 1)//sy
                       * sky_w * 3 +
                       (sky_w - sx - 1)//sx
                       * 3), pano_buffer + (
                       py//py
                       * pano_width * pano_components +
                       px//px
                       * pano_components), 3);

            //up
            //xx//pxf = (fmodf_wrap(atan2f(1 - 2 * syf, 1 - 2 * sxf) / (2 * (float)M_PI), 1)) ;
            pxf = fmodf_wrap((atanf((1 - 2 * syf) / (1 - 2 * sxf)) + ((1 - tmp_a) * sgnf(1 - 2 * syf) * (float)M_PI_2)) / (2 * (float)M_PI), 1);
            px = pxf * pano_width;
            pyf = (fmodf_wrap(acosf(1 / tmp_e) / (float)M_PI, 1)) ;
            py = pyf * pano_height;
            memcpy(sky_u + (
                       (sky_w - sx - 1)//sy
                       * sky_w * 3 +
                       (sky_h - sy - 1)//sx
                       * 3), pano_buffer + (
                       py//py
                       * pano_width * pano_components +
                       px//px
                       * pano_components), 3);

            //down
            //pxf = (fmodf_wrap(atan2f(1 - 2 * syf, 1 - 2 * sxf) / (2 * (float)M_PI), 1)) ;
            //px = pxf * pano_width;
            pyf = (fmodf_wrap(acosf(-1 / tmp_e) / (float)M_PI, 1)) ;
            py = pyf * pano_height;
            memcpy(sky_d + (
                       (sky_w - sx - 1)//sy
                       * sky_w * 3 +
                       (sky_h - sy - 1)//sx
                       * 3), pano_buffer + (
                       py//py
                       * pano_width * pano_components +
                       px//px
                       * pano_components), 3);

        }
    }

#ifdef DEBUG
    //生成完毕计时
    gettimeofday(&tpend, NULL);
    timeuse = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
    timeuse /= 1000000;
    //输出耗时
    fprintf(stderr, "生成完毕计时: %fs\n", timeuse);
    //重新计时开始
    gettimeofday(&tpstart, NULL);
#endif

    //开始保存
    write_JPEG_file("f.jpg", sky_quality, sky_f, sky_w, sky_h);
    write_JPEG_file("b.jpg", sky_quality, sky_b, sky_w, sky_h);
    write_JPEG_file("l.jpg", sky_quality, sky_l, sky_w, sky_h);
    write_JPEG_file("r.jpg", sky_quality, sky_r, sky_w, sky_h);
    write_JPEG_file("u.jpg", sky_quality, sky_u, sky_w, sky_h);
    write_JPEG_file("d.jpg", sky_quality, sky_d, sky_w, sky_h);

#ifdef DEBUG
    //保存结束
    gettimeofday(&tpend, NULL);
    timeuse = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
    timeuse /= 1000000;
    //输出耗时
    fprintf(stderr, "保存完毕计时: %fs\n", timeuse);
#endif

    free(sky_f);
    free(sky_b);
    free(sky_l);
    free(sky_r);
    free(sky_u);
    free(sky_d);
    free(pano_buffer);
    pano_buffer = 0;

    (void)jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);

    return 0;
}
