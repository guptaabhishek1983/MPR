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
        private bool m_isMaximized;

        private Axis m_axis;

        public Axis Axis
        {
            get { return m_axis; }
        }
        private ImagePanel2 m_imagePanel;
        private MPR_UI_Interface m_UIInterface;
        private double m_position;
        private int m_index;


        /// Store structure set contour point with respect to ROINumber.
        private Dictionary<int, Dictionary<int, List<PointF>>> m_roiPoints;
        private Dictionary<int, Color> m_roiColor;
        private Dictionary<int, Dictionary<int, List<PointF>>> m_dosePoints;
        private Dictionary<int, Color> m_doseColor;

        public Dictionary<int, Color> RoiColor
        {
            get { return m_roiColor; }
            set { m_roiColor = value; }
        }

        public Dictionary<int, Color> DoseColor
        {
            get { return m_doseColor; }
            set { m_doseColor = value; }
        }

        public ImageControl()
        {
            InitializeComponent();
        }
        
        public ImageControl(Axis axis)
        {
            InitializeComponent();
            this.m_isMaximized = false;
            this.m_axis = axis;
            this.m_imagePanel = new ImagePanel2();
            this.m_imagePanel.Parent = this;
            this.m_imagePanel.Dock = DockStyle.Fill;
            this.m_imagePanel.EVT_MPRCursorTranslated += TranslateMPRCursor;
            this.m_imagePanel.EVT_RaisePixelIntensity +=RaisePixelIntensity;
            this.m_imagePanel.EVT_RaiseSlicerRotated+=RaiseSlicerRotated;
            this.panel1.Controls.Add(this.m_imagePanel);
            m_UIInterface = MPR_UI_Interface.GetHandle();
            EventHandling.EventHandler1.Instance.EVT_UpdateImage += Handle_UpdateImage;

            m_roiColor = new Dictionary<int, Color>();
            m_roiPoints = new Dictionary<int, Dictionary<int, List<PointF>>>();
            m_doseColor = new Dictionary<int, Color>();
            m_dosePoints = new Dictionary<int, Dictionary<int, List<PointF>>>();
            //m_UIInterface.EVT_UpdateImage += Handle_UpdateImage;
        }

        private void Handle_UpdateImage(BitmapWrapper bmp, int axis, double xPos, double yPos, Dictionary<int, Dictionary<int, List<PointF>>> roiPoints, Dictionary<int, Color> roiColor, BitmapWrapper dose_bmp, double tx, double ty, Dictionary<int, Dictionary<int, List<PointF>>> dosePoints, Dictionary<int, Color> doseColor)
        {
            if (axis == (int)m_axis)
            {
                m_dosePoints = dosePoints;
                m_doseColor = doseColor;
                Handle_UpdateImage(bmp, axis, xPos, yPos, roiPoints, roiColor,dose_bmp,  tx,ty);

            }
        }

        private void Handle_UpdateImage(BitmapWrapper bmp, int axis, double xPos, double yPos, Dictionary<int, Dictionary<int, List<PointF>>> roiPoints, Dictionary<int, Color> roiColor, BitmapWrapper dose_bmp, double tx, double ty)
        {
            if(axis==(int)m_axis)
            {
                if (dose_bmp != null)
                {
                    this.m_imagePanel.DoseBitmap = dose_bmp.StoredBitmap;
                    this.m_imagePanel.DosePosition = new PointF((float)tx, (float)ty);
                }
                else
                {
                    this.m_imagePanel.DoseBitmap = null;
                }
                Handle_UpdateImage(bmp, axis, xPos, yPos, roiPoints, roiColor);

            }
        }

        private void Handle_UpdateImage(BitmapWrapper bmp, int axis, double xPos, double yPos, Dictionary<int, Dictionary<int, List<PointF>>> roiPoints, Dictionary<int, Color> roiColor)
        {
            if (axis == (int)this.m_axis)
            {
                
                Handle_UpdateImage(bmp, axis, xPos, yPos);
                m_roiPoints = roiPoints;
                m_roiColor = roiColor;
            }
            Invalidate();
            Update();
        }

        private void RaiseSlicerRotated(int angle)
        {
            this.m_UIInterface.RotateAxesAlongPlane((int)this.m_axis, angle);
        }

        private void RaisePixelIntensity(Point p)
        {
            this.m_imagePanel.PixelIntensity = this.m_UIInterface.GetPixelIntensity((int)this.m_axis, p.X, p.Y);
        }

        private void Handle_UpdateImage(BitmapWrapper bmpWrapper, int axis, double reslicerPositionX, double reslicerPositionY)
        {
            if (axis == (int)this.m_axis)
            {
                MPR_UI_Interface.WriteLog("Handling update image event.");
                this.m_imagePanel.StoreBitmap = bmpWrapper.StoredBitmap;
                //if(axis == (int)Axis.AxialAxis)
               //     this.m_UIInterface.GetCurrentSliceROI(axis,ref m_roiPoints, ref m_roiColor);
                Position = m_UIInterface.GetCurrentImagePosition((int)this.m_axis);
                Index = m_UIInterface.GetCurrentImageIndex((int)this.m_axis);

                DosePosition = 0;// m_UIInterface.GetCurrentDoseImagePosition((int)this.m_axis);
                DoseIndex = -0;// m_UIInterface.GetCurrentDoseImageIndex((int)this.m_axis);

                //UpdateCursorPosition();
                PointF cursorPoint = new PointF((float)reslicerPositionX, (float)reslicerPositionY);
                switch(m_axis)
                {
                    case Axis.AxialAxis:
                        {
                            this.m_imagePanel.SetCursorPositionX_Axis(cursorPoint, Axis.SagittalAxis);
                            this.m_imagePanel.SetCursorPositionY_Axis(cursorPoint, Axis.CoronalAxis);
                            
                        }
                        break;
                    case Axis.SagittalAxis:
                        {
                            this.m_imagePanel.SetCursorPositionX_Axis(cursorPoint, Axis.CoronalAxis);
                            this.m_imagePanel.SetCursorPositionY_Axis(cursorPoint, Axis.AxialAxis);
                        }
                        break;

                    case Axis.CoronalAxis:
                        {
                            this.m_imagePanel.SetCursorPositionX_Axis(cursorPoint, Axis.SagittalAxis);
                            this.m_imagePanel.SetCursorPositionY_Axis(cursorPoint, Axis.AxialAxis);
                        }
                        break;
                    default:
                        break;
                }
                
                
                
            }
        }

        private void TranslateMPRCursor(Point p)
        {
            m_UIInterface.UpdateSlicerPosition((int)this.m_axis, (float)p.X, (float)p.Y);
            //UpdateCursorPosition();
            Invalidate();
            Update();
        }

        internal void InitScrollBarAndLoadImage()
        {
            this.scrollBar.Maximum = m_UIInterface.GetNumberOfImages((int)this.m_axis);
            this.scrollBar.Minimum = 0;
            this.scrollBar.SmallChange = 1;
            this.scrollBar.LargeChange = 1;
            ScrollBarCurrentVal = this.scrollBar.Value = m_UIInterface.GetCurrentImageIndex((int)this.m_axis);

            // Init scroll bar event
            this.scrollBar.ValueChanged += scrollBar_ValueChanged;
            
            
            // ORIENTATION MARKERS
            OrientationMarkerLeft = m_UIInterface.GetOrientationMarkerLeft((int)this.m_axis);
            OrientationMarkerRight = m_UIInterface.GetOrientationMarkerRight((int)this.m_axis);
            OrientationMarkerTop = m_UIInterface.GetOrientationMarkerTop((int)this.m_axis);
            OrientationMarkerBottom = m_UIInterface.GetOrientationMarkerBottom((int)this.m_axis);
            LoadImage();

            // PIXEL SPACING
            double[] _pixelSpacing = { 0,0,0};
            unsafe
            {
                fixed (double* resPtr = _pixelSpacing)
                {
                    m_UIInterface.GetPixelSpacing((int)this.m_axis, resPtr);
                }
            }
            this.m_imagePanel.XPixelSpacing = _pixelSpacing[0];
            this.m_imagePanel.YPixelSpacing = _pixelSpacing[1];
           // UpdateCursorPosition();
            
        }

        
        void scrollBar_ValueChanged(object sender, EventArgs e)
        {


            m_UIInterface.Scroll((int)this.m_axis, ScrollBarCurrentVal - scrollBar.Value);
            ScrollBarCurrentVal = scrollBar.Value;
        }


        //internal void UpdateCursorPosition()
        //{
        //    switch (this.m_axis)
        //    {
        //        case Axis.AxialAxis:
        //            {
        //                int[] _position = { 0,0};
        //                unsafe
        //                {
        //                    fixed (int* resPtr = _position)
        //                    {
        //                        m_UIInterface.GetCurrentSlicerPositionRelativeToIndex((int)this.m_axis, resPtr);
        //                    }
        //                }

        //                PointF cursorPoint = new PointF((float)_position[0], (float)_position[1]);

        //                this.m_imagePanel.SetCursorPositionY_Axis(cursorPoint, Axis.CoronalAxis);
        //                this.m_imagePanel.SetCursorPositionX_Axis(cursorPoint, Axis.SagittalAxis);
        //            }
        //            break;
        //        case Axis.CoronalAxis:
        //            {
        //                int[] _position = { 0, 0 };
        //                unsafe
        //                {
        //                    fixed (int* resPtr = _position)
        //                    {
        //                        m_UIInterface.GetCurrentSlicerPositionRelativeToIndex((int)this.m_axis, resPtr);
        //                    }
        //                }
        //                PointF cursorPoint = new PointF((float)_position[0], (float)_position[1]);

        //                this.m_imagePanel.SetCursorPositionX_Axis(cursorPoint, Axis.SagittalAxis);
        //                this.m_imagePanel.SetCursorPositionY_Axis(cursorPoint, Axis.AxialAxis);

        //            }
        //            break;
        //        case Axis.SagittalAxis:
        //            {
        //                int[] _position = { 0, 0 };
        //                unsafe
        //                {
        //                    fixed (int* resPtr = _position)
        //                    {
        //                        m_UIInterface.GetCurrentSlicerPositionRelativeToIndex((int)this.m_axis, resPtr);
        //                    }
        //                }
        //                PointF cursorPoint = new PointF((float)_position[0], (float)_position[1]);
        //                this.m_imagePanel.SetCursorPositionX_Axis(cursorPoint, Axis.CoronalAxis);
        //                this.m_imagePanel.SetCursorPositionY_Axis(cursorPoint, Axis.AxialAxis);
        //            }
        //            break;
        //        default:
        //            break;
        //    }
        //}

        internal void LoadImage()
        {
            BitmapWrapper bmpWrapper = m_UIInterface.GetDisplayImage((int)this.m_axis);
            Position = m_UIInterface.GetCurrentImagePosition((int)this.m_axis);
            Index = m_UIInterface.GetCurrentImageIndex((int)this.m_axis);
            //if (Index != scrollBar.Value)
            //    MessageBox.Show("Alert");
            
            //this.m_imagePanel.StoreBitmap = bmpWrapper.StoredBitmap;
            
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

        public int ScrollBarCurrentVal { get; set; }

        internal void StackScrolled(int p)
        {
            ScrollBarCurrentVal = this.scrollBar.Value += p;
            
        }

        public Dictionary<int, Dictionary<int, List<PointF>>> RoiPoints
        {
            get { return  m_roiPoints; } 
        }

        public Dictionary<int, Dictionary<int, List<PointF>>> DosePoints
        {
            get { return m_dosePoints; }
        }

        public double DosePosition { get; set; }

        public int DoseIndex { get; set; }

        private void button1_Click(object sender, EventArgs e)
        {
            this.m_isMaximized = !this.m_isMaximized;

            EventHandling.EventHandler1.Instance.RaisePanelReszing(this, this.m_isMaximized);

        }
    }
}
