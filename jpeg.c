﻿#include "jpeg.h"

static unsigned char g_color_buf[FB_SIZE] = {0};
static int  		 lcd_fd		;
static int 			 *mmap_fd	;

//1、打开显示屏(jpeg)
int Jpeg_Lcd_Open(void)
{
	//a、打开显示屏
	lcd_fd = open("/dev/fb0", O_RDWR);
	if(lcd_fd<0)
	{
		printf("open lcd error\n");
		return -1;
	}
	//b、映射显示屏内存
	mmap_fd  = (int *)mmap(	
						NULL, 					//映射区的开始地址，设置为NULL时表示由系统决定映射区的起始地址
						FB_SIZE, 				//映射区的长度
						PROT_READ|PROT_WRITE, 	//内容可以被读取和写入
						MAP_SHARED,				//共享内存
						lcd_fd, 				//有效的文件描述词
						0						//被映射对象内容的起点
					  );
	return 0;
}

//2、显示jpg图片
int Jpeg_Lcd_Show(unsigned int x,unsigned int y,const char *pjpg_path)  
{
	
	/*定义解码对象，错误处理对象*/
	struct 	jpeg_decompress_struct 	cinfo;
	struct 	jpeg_error_mgr 			jerr;	
	
	unsigned char 	*pcolor_buf = g_color_buf;
	char 	*pjpg;
	
	unsigned int 	i=0;
	unsigned int	color =0;
	//unsigned int	count =0;
	
	unsigned int 	x_s = x;
	unsigned int 	x_e ;	
	unsigned int 	y_e ;
	unsigned int	y_n	= y;
	unsigned int	x_n	= x;
	
			 int	jpg_fd;
	unsigned int 	jpg_size;

	if(pjpg_path!=NULL)
	{
		/* 申请jpg资源，权限可读可写 */	
		jpg_fd=open(pjpg_path,O_RDWR);
		
		if(jpg_fd == -1)
		{
		   printf("open %s error\n",pjpg_path);
		   
		   return -1;	
		}	
		
		/* 获取jpg文件的大小 */
		struct stat statbuff;
		if(stat(pjpg_path, &statbuff) < 0)
			printf("jpg_size=%d\n",  jpg_size);
		else
			jpg_size = statbuff.st_size;
		


		if(jpg_size<3000)
			return -1;
		
		/* 为jpg文件申请内存空间 */	
		pjpg = malloc(jpg_size);

		/* 读取jpg文件所有内容到内存 */		
		read(jpg_fd,pjpg,jpg_size);
	}
	else
		return -1;

	/*注册出错处理*/
	cinfo.err = jpeg_std_error(&jerr);

	/*创建解码*/
	jpeg_create_decompress(&cinfo);

	/*直接解码内存数据*/		
	jpeg_mem_src(&cinfo,pjpg,jpg_size);
	
	/*读文件头*/
	jpeg_read_header(&cinfo, TRUE);

	/*开始解码*/
	jpeg_start_decompress(&cinfo);	
	
	x_e	= x_s +cinfo.output_width;
	y_e	= y  +cinfo.output_height;	

	/*读解码数据*/
	while(cinfo.output_scanline < cinfo.output_height )
	{		
		pcolor_buf = g_color_buf;
		
		/* 读取jpg一行的rgb值 */
		jpeg_read_scanlines(&cinfo,&pcolor_buf,1);
		
		for(i=0; i<cinfo.output_width; i++)
		{
			/* 不显示的部分 */
			/* if(y_n>g_jpg_in_jpg_y && y_n<g_jpg_in_jpg_y+240)
				if(x_n>g_jpg_in_jpg_x && x_n<g_jpg_in_jpg_x+320)
				{
					pcolor_buf +=3;		
					x_n++;			
					continue;
				} */
				
			/* 获取rgb值 */
			color = 		*(pcolor_buf+2);
			color = color | *(pcolor_buf+1)<<8;
			color = color | *(pcolor_buf)<<16;	
			
			/* 显示像素点 */
			*(mmap_fd+y_n*800+x_n)=color;
			
			pcolor_buf +=3;
			
			x_n++;
		}
		
		/* 换行 */
		y_n++;			
		x_n = x_s;
		
	}				
	/*解码完成*/
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	if(pjpg_path!=NULL)
	{
		/* 关闭jpg文件 */
		close(jpg_fd);	
		
		/* 释放jpg文件内存空间 */
		free(pjpg);		
	}
	return 0;
}

//3、显示摄像头捕捉画面
int Jpeg_Lcd_Show_Camera(unsigned int x,unsigned int y,char *pjpg_buf,unsigned int jpg_buf_size)  
{
	/*定义解码对象，错误处理对象*/
	struct 	jpeg_decompress_struct 	cinfo;
	struct 	jpeg_error_mgr 			jerr;	
	
	unsigned char 	*pcolor_buf = g_color_buf;
	char 	*pjpg;
	
	unsigned int 	i=0;
	unsigned int	color =0;
	
	unsigned int 	x_s = x;
	
	pjpg = pjpg_buf;

	/*注册出错处理*/
	cinfo.err = jpeg_std_error(&jerr);

	/*创建解码*/
	jpeg_create_decompress(&cinfo);

	/*直接解码内存数据*/		
	jpeg_mem_src(&cinfo,pjpg,jpg_buf_size);
	
	/*读文件头*/
	jpeg_read_header(&cinfo, TRUE);

	/*开始解码*/
	jpeg_start_decompress(&cinfo);

	/*读解码数据*/
	while(cinfo.output_scanline < cinfo.output_height )
	{		
		pcolor_buf = g_color_buf;
			
		/* 读取jpg一行的rgb值 */
		jpeg_read_scanlines(&cinfo,&pcolor_buf,1);
		jpeg_read_scanlines(&cinfo,&pcolor_buf,1);
		for(i=0; i<cinfo.output_width/2; i++)
		{
			/* 获取rgb值 */
			color = 		*(pcolor_buf+2);
			color = color | *(pcolor_buf+1)<<8;
			color = color | *(pcolor_buf)<<16;	
			/* 显示像素点 */

			*(mmap_fd+y*800+x)=color;
			pcolor_buf +=6;
			x++;

		}
		/* 换行 */
		y++;			
		x = x_s;	
	}		
			
	/*解码完成*/
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	return 0;
}
//4、关闭显示屏(jpeg)
int Jpeg_Lcd_Close(void)
{
	/* 取消内存映射 */
	munmap(mmap_fd, FB_SIZE);

	/* 关闭LCD设备 */
	close(lcd_fd);

	return 0;
}


