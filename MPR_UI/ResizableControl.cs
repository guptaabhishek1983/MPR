using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;

namespace MPR_UI
{
    public class ResizableControl
    {
        #region Maximize/Restore MPR Image panels
        public delegate void PanelResizing(bool fitContainer);
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

        public void RaisePanelReszing(bool fitContainer)
        {
            if (m_panelResizing == null) return;
            m_panelResizing(fitContainer);
        }

        #endregion
    }
}
