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

//#define DSA
#define INTERPOLATION
int _tmain(int argc, _TCHAR* argv[])
{
#ifdef DSA
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

#endif

#ifdef INTERPOLATION

	std::string inputImage = "D:\\DicomDataSet\\Interpolation\\Image_0.dcm";
	// grey 16 bit images
	RadRTDicomInterface* pDicom = new RTDcmtkDicomInterface(inputImage.c_str());
	//RadRTDicomInterface* secondDicom = new RTDcmtkDicomInterface(secondFrame.c_str());
	if (pDicom->IsFileLoadedSuccessfully() == 1)
	{
		image overlay = born_image();
		image input_raw = born_image();
		input_raw.width = pDicom->Get_ROW();
		input_raw.height = pDicom->Get_COLOUMN();
		input_raw.size = input_raw.width*input_raw.height;
		input_raw.type = TYPE_S16Data;
		input_raw.data = rad_get_memory(input_raw.size*rad_sizeof(input_raw.type));

		pDicom->InflateSingleFrameDicomPixelData(&input_raw, &overlay);

		short* _inData = (short*)input_raw.data;

		const int zoom = 2;

		image interpolated_raw = born_image();
		interpolated_raw.width = input_raw.width*zoom;
		interpolated_raw.height = input_raw.height*zoom;
		interpolated_raw.size = interpolated_raw.width*interpolated_raw.height;
		interpolated_raw.type = TYPE_S16Data;
		interpolated_raw.data = rad_get_memory(interpolated_raw.size*rad_sizeof(interpolated_raw.type));

		short* _interpolatedData = (short*)interpolated_raw.data;

		short A, B, C, D, x, y, gray;

		float xRatio = ((float)(input_raw.width - 1)) / interpolated_raw.width;

		float yRatio = ((float)(input_raw.height - 1)) / interpolated_raw.height;

		float x_diff, y_diff;

		int offset = 0;

		for (int i = 0; i < interpolated_raw.height; i++)

		{

			for (int j = 0; j < interpolated_raw.width; j++)

			{

				x = (short)(xRatio * j);

				y = (short)(yRatio * i);

				x_diff = (xRatio * j) - x;

				y_diff = (yRatio * i) - y;

				int index = y * input_raw.width + x;



				// range is 0 to 65535 thus bitwise AND with 0xffff

				A = (short)(_inData[index] & 0xffff);

				B = (short)(_inData[index + 1] & 0xffff);

				C = (short)(_inData[index + input_raw.width] & 0xffff);

				D = (short)(_inData[index + input_raw.width + 1] & 0xffff);

				gray = (short)(

					A * (1 - x_diff) * (1 - y_diff) + B * (x_diff)* (1 - y_diff) +

					C * (y_diff)* (1 - x_diff) + D * (x_diff * y_diff)

					);



				_interpolatedData[offset++] = gray;



			}
		}

		image displayImage = born_image();
		displayImage.width = interpolated_raw.width;
		displayImage.height = interpolated_raw.height;
		displayImage.size = interpolated_raw.size;
		displayImage.type = TYPE_U8Data;

		displayImage.data = rad_get_memory(displayImage.height*displayImage.width*rad_sizeof(displayImage.type));

		voi_lut_transform_image_fast(displayImage, interpolated_raw, 400, 40,
			0, 255,
			1.0,
			-1024);

		u_int width = displayImage.width;
		u_int height = displayImage.height;


		FILE *fp = fopen("D:\\Interpolated.ppm", "wb");

		unsigned char *p3;
		p3 = (unsigned char *)displayImage.data;

		fprintf(fp, "P3\n%d %d\n255", width, height);
		u_int ix, iy;
		for (iy = 0; iy < height; iy++)
			for (ix = 0; ix < width; ix++)	{
				if (((iy*width) + ix) % 5 == 0)
					fprintf(fp, "\n");
				fprintf(fp, "%li ", p3[iy*displayImage.width + ix]);
				fprintf(fp, "%li ", p3[iy*displayImage.width + ix]);
				fprintf(fp, "%li ", p3[iy*displayImage.width + ix]);
			}
		fclose(fp);


		// ushort[] temp = new ushort[targetWidth * targetHeight];

	}

#endif

	return 0;
}

