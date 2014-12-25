using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using ImageUtils;

namespace EventHandling
{
    public class EventHandler1
    {
        private static EventHandler1 instance;
        private EventHandler1() { }
        public static EventHandler1 Instance
        {
            get
            {
                if (instance == null)
                {
                    instance = new EventHandler1();
                }
                return instance;
            }
        }
        #region UpdateImage
        public delegate void UpdateImage(BitmapWrapper bmp, int axis, double xPos, double yPos, Dictionary<int, Dictionary<int, List<PointF>>> roiPoints, Dictionary<int, Color> roiColor, BitmapWrapper dose_bmp, double tx, double ty, Dictionary<int, Dictionary<int, List<PointF>>> dosePoints, Dictionary<int, Color> doseColor);
        private UpdateImage m_updateImage;

        public event UpdateImage EVT_UpdateImage
        {
            [MethodImpl(MethodImplOptions.Synchronized)]
            add
            {
                m_updateImage = (UpdateImage)Delegate.Combine(m_updateImage, value);
            }
            [MethodImpl(MethodImplOptions.Synchronized)]
            remove
            {
                m_updateImage = (UpdateImage)Delegate.Remove(m_updateImage, value);
            }

        }

        public void RaiseUpdateImage(BitmapWrapper bmp, int axis, double xPos, double yPos, Dictionary<int, Dictionary<int, List<PointF>>> roiPoints, Dictionary<int, Color> roiColor, BitmapWrapper dose_bmp, double tx, double ty, Dictionary<int, Dictionary<int, List<PointF>>> dosePoints, Dictionary<int, Color> doseColor)
        {
            if (m_updateImage == null) return;
            m_updateImage(bmp, axis, xPos, yPos, roiPoints, roiColor,dose_bmp, tx,ty, dosePoints, doseColor);
            //var receivers = m_updateImage.GetInvocationList();

            //foreach (UpdateImage receiver in receivers)
            //{
            //    receiver.BeginInvoke(bmp, axis, xPos, yPos, roiPoints, roiColor, null, null);
            //}
        }
        #endregion

        #region CursorTranslation
        public delegate void CursorTranslationCompleted();
        private CursorTranslationCompleted m_cursorTranslationCompleted;
        public event CursorTranslationCompleted EVT_CursorTranslationCompleted
		{
        [MethodImpl(MethodImplOptions.Synchronized)]
			add{  
                m_cursorTranslationCompleted = (CursorTranslationCompleted)Delegate.Combine(m_cursorTranslationCompleted, value); }
    [MethodImpl(MethodImplOptions.Synchronized)]
			remove{  
                m_cursorTranslationCompleted = (CursorTranslationCompleted)Delegate.Remove(m_cursorTranslationCompleted, value); }
			
		}

    public void RaiseCursorTranslationCompleted()
    {
        if (m_cursorTranslationCompleted != null)
		{
			m_cursorTranslationCompleted();
		}
    }
        #endregion

        #region Maximize/Restore MPR Image panels
    public delegate void PanelResizing(object sender, bool state);
    private PanelResizing m_panelResizing;
    public event PanelResizing EVT_PanelResizing
    {
        [MethodImpl(MethodImplOptions.Synchronized)]
        add
        {
            m_panelResizing = (PanelResizing)Delegate.Combine(m_panelResizing, value);
        }
        [MethodImpl(MethodImplOptions.Synchronized)]
        remove
        {
            m_panelResizing = (PanelResizing)Delegate.Remove(m_panelResizing, value);
        }

    }

    public void RaisePanelReszing(object sender, bool state)
    {
        if (m_panelResizing == null) return;
        m_panelResizing(sender, state);
    }

        #endregion
    }
}
