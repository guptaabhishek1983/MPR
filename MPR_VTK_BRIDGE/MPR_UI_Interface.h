// MPR_VTK_BRIDGE.h

#pragma once

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Drawing;
#include "MPR.h"
#include "RTSS.h"
#include "DCMGeometry.h"
#include "Dose.h"
using namespace RTViewer;
using namespace ImageUtils;
namespace MPR_VTK_BRIDGE {

	public ref class MPR_UI_Interface
	{
		
	private: // members
		static MPR_UI_Interface^ m_handle;
		MPR* m_mpr;
		RTStructureSet* m_rtss;
		Plan* m_plan;
		Dose* m_dose;

	public: // methods
		static MPR_UI_Interface^ GetHandle();
		~MPR_UI_Interface(void);
		void InitMPR(String^ path);
		BitmapWrapper^ GetDisplayImage(int axis);
		int GetNumberOfImages(int axis);
		void Scroll(int axis, int delta);
		int GetCurrentImageIndex(int axis);
		double GetCurrentImagePosition(int axis);

		int GetCurrentDoseImageIndex(int axis);
		double GetCurrentDoseImagePosition(int axis);

		void GetCurrentSlicerPositionRelativeToIndex(int axis, int* pos);
		void UpdateSlicerPosition(int axis, float x, float y);
		String^ GetOrientationMarkerLeft(int axis);
		String^ GetOrientationMarkerRight(int axis);
		String^ GetOrientationMarkerTop(int axis);
		String^ GetOrientationMarkerBottom(int axis);
		void GetPixelSpacing(int axis, double* pixelSpacing);
		long int GetPixelIntensity(int axis, int x_pos, int y_pos);
		void RotateAxesAlongPlane(int axis, int angle);

		void GetCurrentSliceROI(int axis, Dictionary<int, Dictionary<int, List<PointF>^>^>^% roiPoints,
			Dictionary<int, Color>^% roiColor);

		void GetDVHData(Dictionary<int, List<double>^>^% dvhPoints, Dictionary<int, List<double>^>^% computed_dvhPoints);
		int GetTragetPrescribedDose();

	public: //delegates
		//delegate void CursorTranslationCompleted();
		//delegate void UpdateImage(BitmapWrapper^ bmpWrapper, int axis, double reslicerPositionX, double reslicerPositionY);

	public: //static methods
		static void WriteLog(String^ msg);
	public: // event implementation
		/*event CursorTranslationCompleted^ EVT_CursorTranslationCompleted
		{
			void add(CursorTranslationCompleted^ p){ m_cursorTranslationCompleted += p; }
			void remove(CursorTranslationCompleted^ p){ m_cursorTranslationCompleted -= p; }
			void raise()
			{
				if (m_cursorTranslationCompleted != nullptr)
				{
					m_cursorTranslationCompleted();
				}
			}
		}*/

//		event UpdateImage^ EVT_UpdateImage
//		{
//			void add(UpdateImage^ p){ m_updateImage += p; }
//			void remove(UpdateImage^ p){ m_updateImage -= p; }
//			void raise(BitmapWrapper^ bmp, int axis, double reslicerPositionX, double reslicerPositionY)
//			{
//				if (m_updateImage != nullptr)
//				{
//					array<System::Delegate^>^ receivers = m_updateImage->GetInvocationList();
//					for (int i = 0; i < receivers->Length; i++)
//					{
////						receivers[i]->DynamicInvoke(args);
//						//m_updateImage->BeginInvoke(bmp, axis, reslicerPositionX, reslicerPositionY, nullptr, nullptr);
//					}
//					
//				}
//			}
//		}

	private: //events
		//CursorTranslationCompleted^ m_cursorTranslationCompleted;
		//UpdateImage^ m_updateImage;

	protected:
		MPR_UI_Interface(void);
	};
}
