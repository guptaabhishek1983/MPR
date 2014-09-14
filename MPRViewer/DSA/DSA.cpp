// DSA.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include "rad_util.h"
#include "rad_logger.h"
#include "RTDcmtkDicomInterface.h"
#include "rad_template_helper.h"

#include "streaming-image.h"

using namespace std;
#define __FILENAME__ "DSA.cpp"

int _tmain(int argc, _TCHAR* argv[])
{
	std::string firstFrame = "D:\\DicomDataSet\\XA\\Dataset1\\wwwl.dcm";
	//std::string secondFrame = "D:\\DicomDataSet\\XA\\Dataset1\\IM9.dcm";

	RadRTDicomInterface* firstDicom = new RTDcmtkDicomInterface(firstFrame.c_str());
	//RadRTDicomInterface* secondDicom = new RTDcmtkDicomInterface(secondFrame.c_str());
	if (firstDicom->IsFileLoadedSuccessfully() == 1 )//&& secondDicom->IsFileLoadedSuccessfully() == 1)
	{

		image overlay = born_image();
		image i1 = born_image();
		i1.width = firstDicom->Get_ROW();
		i1.height = firstDicom->Get_COLOUMN();
		i1.size = i1.width*i1.height;
		i1.type = TYPE_U16Data;
		i1.data = rad_get_memory(i1.size*rad_sizeof(i1.type));

		firstDicom->InflateSingleFrameDicomPixelData(&i1, &overlay);
		
		/*{
			u_int width = i1.width;
			u_int height = i1.height;


			FILE *fp = fopen("D:\\XA\\ww.ppm", "wb");

			U16Data p3 = (U16Data)i1.data;

			fprintf(fp, "P3\n%d %d\n255", width, height);
			u_int x, y;
			for (y = 0; y < height; y++)
				for (x = 0; x < width; x++)	{
					if (((y*width) + x) % 5 == 0)
						fprintf(fp, "\n");
					fprintf(fp, "%li ", p3[y*i1.width + x]);
					fprintf(fp, "%li ", p3[y*i1.width + x]);
					fprintf(fp, "%li ", p3[y*i1.width + x]);
				}
			fclose(fp);
		}
*/
		/*image i2 = born_image();
		i2.width = 1024;
		i2.height = 1024;
		i2.size = i2.width*i2.height;
		i2.type = TYPE_U16Data;
		i2.data = rad_get_memory(i2.size*rad_sizeof(i2.type));

		secondDicom->InflateSingleFrameDicomPixelData(&i2, &overlay);

		{
			u_int width = i2.width;
			u_int height = i2.height;


			FILE *fp = fopen("D:\\XA\\second.ppm", "wb");

			U16Data p3 = (U16Data)i2.data;

			fprintf(fp, "P3\n%d %d\n255", width, height);
			u_int x, y;
			for (y = 0; y < height; y++)
				for (x = 0; x < width; x++)	{
					if (((y*width) + x) % 5 == 0)
						fprintf(fp, "\n");
					fprintf(fp, "%li ", p3[y*i2.width + x]);
					fprintf(fp, "%li ", p3[y*i2.width + x]);
					fprintf(fp, "%li ", p3[y*i2.width + x]);
				}
			fclose(fp);
		}*/

/*
		image i3 = born_image();
		i3.width = 1024;
		i3.height = 1024;
		i3.size = i3.width*i3.height;
		i3.type = TYPE_S16Data;
		i3.data = rad_get_memory(i3.size*rad_sizeof(i3.type));
		
		S16Data substracted_image = (S16Data)i3.data;*/
		double low=0, high=0;

		U16Data data = (U16Data)i1.data;
		for (int i = 0; i < i1.size; i++)
		{
			if (i == 0)
			{
				low = data[i];
				high = data[i];
			}

			if (low > data[i] && data[i]!=0) 
			{
				low = data[i]; 
			}
			if (high < data[i]) 
			{
				high = data[i]; 
			}

		}
		cout << low << ":" << high << endl;
		image displayImage = born_image();
		displayImage.width = i1.width;
		displayImage.height = i1.height;
		displayImage.size = i1.size;
		displayImage.type = TYPE_U8Data;

		displayImage.data = rad_get_memory(displayImage.height*displayImage.width*rad_sizeof(displayImage.type));
		
		double ww = high - low;
		double wl = (low + high) / 2;
		cout << ww << ":" << wl << endl;
		voi_lut_transform_image_fast(displayImage, i1, ww, wl,
			0, 255,
			1.0,
			0.0);

		u_int width = displayImage.width;
		u_int height = displayImage.height;


		FILE *fp = fopen("D:\\Substracted.ppm", "wb");
		
		unsigned char *p3;
		p3 = (unsigned char *)displayImage.data;

		fprintf(fp, "P3\n%d %d\n255", width, height);
		u_int x, y;
		for (y = 0; y<height; y++)
			for (x = 0; x<width; x++)	{
				if (((y*width) + x) % 5 == 0)
					fprintf(fp, "\n");
				fprintf(fp, "%li ", p3[y*displayImage.width + x]);
				fprintf(fp, "%li ", p3[y*displayImage.width + x]);
				fprintf(fp, "%li ", p3[y*displayImage.width + x]);
			}
		fclose(fp);
		
	}

	

	return 0;
}

