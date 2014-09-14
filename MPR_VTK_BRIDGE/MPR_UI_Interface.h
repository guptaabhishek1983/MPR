// MPR_VTK_BRIDGE.h

#pragma once

using namespace System;
#include "MPR.h"

using namespace RTViewer;
using namespace ImageUtils;
namespace MPR_VTK_BRIDGE {

	public ref class MPR_UI_Interface
	{
		
	private: // members
		static MPR_UI_Interface^ m_handle;
		MPR* m_mpr;

	public: // methods
		static MPR_UI_Interface^ GetHandle();
		~MPR_UI_Interface(void);
		void InitMPR(String^ path);
		BitmapWrapper^ GetDisplayImage(int axis);
		int GetNumberOfImages(int axis);
		void Scroll(int axis, int delta);
		int GetCurrentImageIndex(int axis);
		double GetCurrentImagePosition(int axis);
		double GetCurrentImagePositionRelativeToOrigin(int axis);
		void UpdateSlicerPosition(int axis, float x, float y);
		String^ GetOrientationMarkerLeft(int axis);
		String^ GetOrientationMarkerRight(int axis);
		String^ GetOrientationMarkerTop(int axis);
		String^ GetOrientationMarkerBottom(int axis);

	public: //delegates
		delegate void CursorTranslationCompleted();
		delegate void UpdateImage(BitmapWrapper^ bmpWrapper, int axis, double reslicerPositionX, double reslicerPositionY);

	public: //static methods
		static void WriteLog(String^ msg);
	public: // event implementation
		event CursorTranslationCompleted^ EVT_CursorTranslationCompleted
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
		}

		event UpdateImage^ EVT_UpdateImage
		{
			void add(UpdateImage^ p){ m_updateImage += p; }
			void remove(UpdateImage^ p){ m_updateImage -= p; }
			void raise(BitmapWrapper^ bmp, int axis, double reslicerPositionX, double reslicerPositionY)
			{
				if (m_updateImage != nullptr)
				{
					m_updateImage(bmp, axis, reslicerPositionX, reslicerPositionY);
				}
			}
		}

	private: //events
		CursorTranslationCompleted^ m_cursorTranslationCompleted;
		UpdateImage^ m_updateImage;

	protected:
		MPR_UI_Interface(void);
	};
}
