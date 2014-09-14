using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using MPR_VTK_BRIDGE;
using ImageUtils;
using System.Threading;
using System.Reflection;

namespace MPR_UI
{
    public partial class ImageControl : UserControl
    {
        private Axis m_axis;
        private ImagePanel2 m_imagePanel;
        private MPR_UI_Interface m_UIInterface;
        private double m_position;
        private int m_index;


        public ImageControl()
        {
            InitializeComponent();
        }
        
        public ImageControl(Axis axis)
        {
            InitializeComponent();
            this.m_axis = axis;
            this.m_imagePanel = new ImagePanel2();
            this.m_imagePanel.Dock = DockStyle.Fill;
            this.m_imagePanel.EVT_MPRCursorTranslated += TranslateMPRCursor;
            this.panel1.Controls.Add(this.m_imagePanel);
            m_UIInterface = MPR_UI_Interface.GetHandle();
            m_UIInterface.EVT_UpdateImage += Handle_UpdateImage;
        }

        private void Handle_UpdateImage(BitmapWrapper bmpWrapper, int axis, double reslicerPositionX, double reslicerPositionY)
        {
            if (axis == (int)this.m_axis)
            {
                MPR_UI_Interface.WriteLog("Handling update image event.");
                this.m_imagePanel.StoreBitmap = bmpWrapper.StoredBitmap;
                UpdateCursorPosition();
                Invalidate();
                Update();
            }
        }

        private void TranslateMPRCursor(Point p)
        {
            m_UIInterface.UpdateSlicerPosition((int)this.m_axis, (float)p.X, (float)p.Y);
            UpdateCursorPosition();
            Invalidate();
            Update();
        }

        internal void InitScrollBarAndLoadImage()
        {
            this.scrollBar.Maximum = m_UIInterface.GetNumberOfImages((int)this.m_axis);
            this.scrollBar.Minimum = 0;
            this.scrollBar.SmallChange = 1;
            this.scrollBar.LargeChange = 1;
            this.scrollBar.Value = m_UIInterface.GetCurrentImageIndex((int)this.m_axis);

            // Init scroll bar event
            this.scrollBar.ValueChanged += scrollBar_ValueChanged;

            
            // ORIENTATION MARKERS
            OrientationMarkerLeft = m_UIInterface.GetOrientationMarkerLeft((int)this.m_axis);
            OrientationMarkerRight = m_UIInterface.GetOrientationMarkerRight((int)this.m_axis);
            OrientationMarkerTop = m_UIInterface.GetOrientationMarkerTop((int)this.m_axis);
            OrientationMarkerBottom = m_UIInterface.GetOrientationMarkerBottom((int)this.m_axis);
            LoadImage();
            
        }

        
        void scrollBar_ValueChanged(object sender, EventArgs e)
        {

            if (scrollBar.Value != Index)
            {
                m_UIInterface.Scroll((int)this.m_axis, scrollBar.Value - Index);
                LoadImage();
            }
        }


        internal void UpdateCursorPosition()
        {
            switch (this.m_axis)
            {
                case Axis.AxialAxis:
                    {
                        double PositionS = m_UIInterface.GetCurrentImagePositionRelativeToOrigin((int)Axis.SagittalAxis);
                        double PositionC = m_UIInterface.GetCurrentImagePositionRelativeToOrigin((int)Axis.CoronalAxis);
                        PointF cursorPoint = new PointF((float)PositionS, (float)PositionC);

                        this.m_imagePanel.SetCursorPositionY_Axis(cursorPoint, Axis.CoronalAxis);
                        this.m_imagePanel.SetCursorPositionX_Axis(cursorPoint, Axis.SagittalAxis);
                    }
                    break;
                case Axis.CoronalAxis:
                    {
                        double PositionA = m_UIInterface.GetCurrentImagePositionRelativeToOrigin((int)Axis.AxialAxis);
                        double PositionS = m_UIInterface.GetCurrentImagePositionRelativeToOrigin((int)Axis.SagittalAxis);

                        PointF cursorPoint = new PointF((float)PositionS, (float)PositionA);
                        this.m_imagePanel.SetCursorPositionX_Axis(cursorPoint, Axis.SagittalAxis);
                        this.m_imagePanel.SetCursorPositionY_Axis(cursorPoint, Axis.AxialAxis);

                    }
                    break;
                case Axis.SagittalAxis:
                    {
                        double PositionA = m_UIInterface.GetCurrentImagePositionRelativeToOrigin((int)Axis.AxialAxis);
                        double PositionC = m_UIInterface.GetCurrentImagePositionRelativeToOrigin((int)Axis.CoronalAxis);

                        PointF cursorPoint = new PointF((float)PositionC, (float)PositionA);
                        this.m_imagePanel.SetCursorPositionX_Axis(cursorPoint, Axis.CoronalAxis);
                        this.m_imagePanel.SetCursorPositionY_Axis(cursorPoint, Axis.AxialAxis);
                    }
                    break;
                default:
                    break;
            }
        }

        internal void LoadImage()
        {
            BitmapWrapper bmpWrapper = m_UIInterface.GetDisplayImage((int)this.m_axis);
            Position = m_UIInterface.GetCurrentImagePosition((int)this.m_axis);
            Index = m_UIInterface.GetCurrentImageIndex((int)this.m_axis);
            //if (Index != scrollBar.Value)
            //    MessageBox.Show("Alert");
            this.m_imagePanel.StoreBitmap = bmpWrapper.StoredBitmap;
            UpdateCursorPosition();
            this.Invalidate();
            
        }

        internal int GetScrollbarValue()
        {
            return scrollBar.Value;
        }

        internal double Position
        {
            get { return m_position; }
            set { m_position = value; }
        }

        internal int Index
        {
            get { return m_index; }
            set { m_index = value; }
        }

        internal string OrientationMarkerLeft { set; get; }
        internal string OrientationMarkerRight { set; get; }
        internal string OrientationMarkerBottom { set; get; }
        internal string OrientationMarkerTop { set; get; }

        internal void UpdateCursor(System.Windows.Forms.Cursor cursor)
        {
            this.Cursor = cursor;
        }
    }
}
