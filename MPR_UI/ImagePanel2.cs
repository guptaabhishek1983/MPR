using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using ImageUtils;
using MPR_UI.Properties;
using MPR_VTK_BRIDGE;
namespace MPR_UI
{
    public partial class ImagePanel2 : Control
    {
        public delegate void MPRCursorTranslated(Point p);
        public event MPRCursorTranslated EVT_MPRCursorTranslated;

        public delegate void RaisePixelIntensity(Point p);
        public event RaisePixelIntensity EVT_RaisePixelIntensity;

        public delegate void RaiseSlicerRotated(int angle);
        public event RaiseSlicerRotated EVT_RaiseSlicerRotated;
        private Bitmap m_storedBitmap;
        private struct MPRCursor
        {
            public Line l1;
            public Line l2;
        };

        private MPRCursor m_mprCursor;
        private CoordinateMapping m_coordinateMapping;
        private PointF cursorPosition;
        private GraphicsPath cursorPath;
        private GraphicsPath XAxisLine;
        private GraphicsPath YAxisLine;
        private Point lastMousePosition;
        private Point lastMousePositionORG;
        // testing
        GraphicsPath objectPath;
        Point objectLocation;
        bool objectSelected;

        
        public ImagePanel2()
        {
            InitializeComponent();

            currentDisplayOffsetPt = new Point(0, 0);
            originalDisplayOffsetPt = new Point(0, 0);
            BorderSize = 2;
            currentZoomFactor = 1.0F;
            originalZoomFactor = 1.0F;
            lastMousePosition = new Point(0, 0);

            // initialize MPR cursor
            this.m_mprCursor = new MPRCursor();
            // initial coordinate system mapping
            this.m_coordinateMapping = new CoordinateMapping();
            // cursor path
            cursorPath = new GraphicsPath();
            cursorPath.Reset();

            XAxisLine = new GraphicsPath();
            XAxisLine.Reset();

            YAxisLine = new GraphicsPath();
            YAxisLine.Reset();


            // testing
            objectPath = new GraphicsPath();
            objectLocation = new Point(0, 0);
            objectPath.Reset();
            objectPath.AddEllipse(objectLocation.X - 5.0F, objectLocation.Y - 5.0F, 10.0F, 10.0F);
            objectPath.CloseFigure();

            // Set few control option.
            SetStyle(ControlStyles.UserPaint, true);
            SetStyle(ControlStyles.AllPaintingInWmPaint, true);
            SetStyle(ControlStyles.DoubleBuffer, true);
            SetStyle(ControlStyles.UserMouse, true);

            // handle resize
            this.Resize += new EventHandler(ImagePanel2_Resize);
        }

        /// <summary>
        /// Handle Image panel resize.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ImagePanel2_Resize(object sender, EventArgs e)
        {
            InitializeZoomAndPosition();
        }

        public ImagePanel2(IContainer container)
        {
            container.Add(this);

            InitializeComponent();
        }

        
        public Bitmap StoreBitmap
        {
            set
            {
                if (m_storedBitmap == null)
                {
                    m_storedBitmap = value;
                    InitializeZoomAndPosition();
                }
                m_storedBitmap = value;
                Invalidate();
            }
            get { return m_storedBitmap; }
        }
        private void InitializeZoomAndPosition()
        {
            if (StoreBitmap == null) return;

            float zoom = 1.0F;
            Point p = new Point(0, 0);
            Size effectivePanelSize = this.Size - new Size((10 + (int)(2 * this.BorderSize)), (10 + (int)(2 * this.BorderSize)));
            float diffHeight = ((float)effectivePanelSize.Height) / ((float)StoreBitmap.Height);
            float diffWidth = ((float)effectivePanelSize.Width) / ((float)StoreBitmap.Width);
            zoom = Math.Min(diffHeight, diffWidth);

            int w = (int)(StoreBitmap.Width * zoom);
            int h = (int)(StoreBitmap.Height * zoom);
            p.X = (int)((effectivePanelSize.Width - w) / (2 * zoom));
            p.X += 5 + (int)BorderSize;
            p.Y = (int)((effectivePanelSize.Height - h) / (2 * zoom));
            p.Y += 5 + (int)BorderSize;
            this.currentDisplayOffsetPt = p;
            this.originalDisplayOffsetPt = p;
            this.currentZoomFactor = zoom;
            this.originalZoomFactor = zoom;
            imageRect = new RectangleF((currentZoomFactor * currentDisplayOffsetPt.X) - 1,
                    (currentZoomFactor * currentDisplayOffsetPt.Y) - 1,
                    (currentZoomFactor * StoreBitmap.Width) + 2,
                    (currentZoomFactor * StoreBitmap.Height) + 2);


        }

        protected override void OnMouseWheel(MouseEventArgs e)
        {
            base.OnMouseWheel(e);
            if(e.Delta>1)
            {

            }
            else
            {

            }
        }

        protected override void OnMouseDown(MouseEventArgs e)
        {
            base.OnMouseDown(e);
            //EVT_MPRCursorTranslated(new Point((int)this.m_mprCursor.l1.P1.X + 1, (int)this.m_mprCursor.l2.P1.Y + 1));

            if (e.Button == System.Windows.Forms.MouseButtons.Left)
            {
                if (objectPath.GetBounds().Contains(e.Location))
                {
                    objectSelected = true;
                }
                objectLocation = e.Location;
            }

            if (e.Button == System.Windows.Forms.MouseButtons.Left)
            {
                if (cursorPath.GetBounds().Contains(e.Location))
                {
                    MPRCursorSelected = true;
                }
                else
                {
                    MPRCursorSelected = false;
                }
            }

            if (e.Button == System.Windows.Forms.MouseButtons.Left)
            {
                if (XAxisLine.GetBounds().Contains(e.Location))
                {
                    MPRAxisSelected = true;
                }

                if (YAxisLine.GetBounds().Contains(e.Location))
                {
                    MPRAxisSelected = true;
                }
            }

            if (e.Button == System.Windows.Forms.MouseButtons.Right)
            {
                this.m_mprCursor.l1.P1 = this.m_coordinateMapping.RotatePoint(this.m_mprCursor.l1.P1, this.cursorPosition, 10);

                this.m_mprCursor.l1.P2 = this.m_coordinateMapping.RotatePoint(this.m_mprCursor.l1.P2, this.cursorPosition, 10);

                this.m_mprCursor.l2.P1 = this.m_coordinateMapping.RotatePoint(this.m_mprCursor.l2.P1, this.cursorPosition, 10);

                this.m_mprCursor.l2.P2 = this.m_coordinateMapping.RotatePoint(this.m_mprCursor.l2.P2, this.cursorPosition, 10);

                if(EVT_RaiseSlicerRotated!=null)
                {
                    EVT_RaiseSlicerRotated(10);
                }

            }
            // update last mouse position
            this.lastMousePositionORG = new Point(e.X, e.Y) ;
            this.lastMousePosition = this.GetOriginalCoords(e.Location);
           
        }
        protected override void OnMouseUp(MouseEventArgs e)
        {
            base.OnMouseUp(e);

            MPRCursorSelected = false;
            
            // testing
            objectSelected = false;

            // update last mouse position
            this.lastMousePositionORG = new Point(e.X, e.Y);
            this.lastMousePosition = this.GetOriginalCoords(e.Location);
            
        }
        
        protected override void OnMouseMove(MouseEventArgs e)
        {
            base.OnMouseMove(e);
            
            
            Point p = this.GetOriginalCoords(new Point(e.X, e.Y));
            if (e.Button == System.Windows.Forms.MouseButtons.Left && MPRCursorSelected == true) 
            {
                
                //Point pDiff = new Point(e.X - lastMousePositionORG.X, e.Y - lastMousePositionORG.Y);
                EVT_MPRCursorTranslated(new Point((int)(p.X*XPixelSpacing), (int)(p.Y*YPixelSpacing)));
            }

            

            if(EVT_RaisePixelIntensity!=null)
            {
                EVT_RaisePixelIntensity(new Point((int)(p.X ), (int)(p.Y)));
            }
            if (objectSelected == true)
            {
                objectLocation = e.Location;
                objectPath.Reset();
                objectPath.AddEllipse(objectLocation.X - 5.0F, objectLocation.Y - 5.0F, 10.0F, 10.0F);
                objectPath.CloseFigure();
            }
            // update last mouse position
            this.lastMousePositionORG = new Point(e.X, e.Y);
            this.lastMousePosition = this.GetOriginalCoords(e.Location);
            Invalidate();
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            if (StoreBitmap == null) return;
            MPR_UI_Interface.WriteLog("Handling paint event.");

            RectangleF srcRect = new RectangleF((e.ClipRectangle.X / currentZoomFactor) - currentDisplayOffsetPt.X,
                    (e.ClipRectangle.Y / currentZoomFactor) - currentDisplayOffsetPt.Y,
                    e.ClipRectangle.Width / currentZoomFactor, e.ClipRectangle.Height / currentZoomFactor);

            Rectangle roundedRectangle = Rectangle.Round(imageRect);
            e.Graphics.DrawRectangle(new Pen(Color.FromArgb(255, 0, 0)), roundedRectangle);
            e.Graphics.InterpolationMode = InterpolationMode.HighQualityBicubic;
            e.Graphics.SmoothingMode = SmoothingMode.AntiAlias;

            e.Graphics.DrawImage(StoreBitmap, e.ClipRectangle, srcRect, GraphicsUnit.Pixel);

            e.Graphics.InterpolationMode = InterpolationMode.HighQualityBicubic;
            e.Graphics.SmoothingMode = SmoothingMode.AntiAlias;
            Pen p = new Pen(Color.Gold, 10.0F);
            e.Graphics.FillPath(p.Brush, objectPath);

            Pen _pen1 = new Pen(Color.LightGoldenrodYellow, 2.0F);
            Pen _pen2 = new Pen(Color.ForestGreen, 2.0F);
            Font _font = new Font("Verdana", 10.0F);
            int spacing = 2;
            int currX = spacing;
            int currY = 4 + spacing;
            SizeF sz = new SizeF(0F, 0F);
            if (this.Parent.Parent.Name.CompareTo("ImageControl") == 0)
            {
                var imgControl = (ImageControl)this.Parent.Parent;
                int idx = imgControl.GetScrollbarValue();

                StringBuilder _sb = new StringBuilder();
                _sb.Append("Scroll Pos#");
                _sb.Append(idx);
                sz = e.Graphics.MeasureString(_sb.ToString(), _font);
                e.Graphics.DrawString(_sb.ToString(), _font, _pen1.Brush, new PointF(currX, currY));
                currY = currY + (int)sz.Height + spacing;

                _sb.Clear();
                _sb.Append("Slicer Pos#");
                _sb.Append(imgControl.Position);
                sz = e.Graphics.MeasureString(_sb.ToString(), _font);
                e.Graphics.DrawString(_sb.ToString(), _font, _pen1.Brush, new PointF(currX, currY));
                currY = currY + (int)sz.Height + spacing;

                _sb.Clear();
                _sb.Append("Slicer idx#");
                _sb.Append(imgControl.Index);
                sz = e.Graphics.MeasureString(_sb.ToString(), _font);
                e.Graphics.DrawString(_sb.ToString(), _font, _pen1.Brush, new PointF(currX, currY));
                currY = currY + (int)sz.Height + spacing;

                
                
            }

            // paint bottom left
            currY = this.Height - 10 - spacing;
            if (this.Parent.Parent.Name.CompareTo("ImageControl") == 0)
            {
                var imgControl = (ImageControl)this.Parent.Parent;

                StringBuilder _sb = new StringBuilder();
                _sb.Clear();
                _sb.Append("X: ").Append(lastMousePosition.X).Append("px Y:").Append(lastMousePosition.Y).Append(" px"); ;
                _sb.Append("/");
                _sb.Append("X: ").Append((int)(lastMousePosition.X*XPixelSpacing)).Append(" mm Y:").Append((int)(lastMousePosition.Y*YPixelSpacing)).Append(" mm");
                sz = e.Graphics.MeasureString(_sb.ToString(), _font);
                currY = currY - (int)sz.Height;
                e.Graphics.DrawString(_sb.ToString(), _font, _pen1.Brush, new PointF(currX, currY));

                _sb.Clear();
                _sb.Append("Value:").Append(PixelIntensity);
                sz = e.Graphics.MeasureString(_sb.ToString(), _font);
                currY = currY - (int)sz.Height;
                e.Graphics.DrawString(_sb.ToString(), _font, _pen1.Brush, new PointF(currX, currY));
            }

            if (cursorPosition != null)
            {
                cursorPath.Reset();
                cursorPath.AddEllipse(cursorPosition.X - 5.0F, cursorPosition.Y - 5.0F, 10.0F, 10.0F);
                cursorPath.CloseFigure();
                e.Graphics.FillPath(p.Brush, cursorPath);
            }
            
            // paint cursor
            if (this.m_mprCursor.l1 != null && XAxisLine!=null)
            {
                XAxisLine.Reset();
                XAxisLine.AddLine(this.m_mprCursor.l1.P1, this.m_mprCursor.l1.P2);
                XAxisLine.CloseFigure();
                e.Graphics.DrawPath(this.m_mprCursor.l1.DisplayPen, XAxisLine);
                //e.Graphics.DrawLine(this.m_mprCursor.l1.DisplayPen,                    this.m_mprCursor.l1.P1.X, this.m_mprCursor.l1.P1.Y, this.m_mprCursor.l1.P2.X, this.m_mprCursor.l1.P2.Y);
            }
            if (this.m_mprCursor.l2 != null && YAxisLine != null)
            {
                YAxisLine.Reset();
                YAxisLine.AddLine(this.m_mprCursor.l2.P1, this.m_mprCursor.l2.P2);
                YAxisLine.CloseFigure();
                e.Graphics.DrawPath(this.m_mprCursor.l2.DisplayPen, YAxisLine);
                //e.Graphics.DrawLine(this.m_mprCursor.l2.DisplayPen,                    this.m_mprCursor.l2.P1.X, this.m_mprCursor.l2.P1.Y, this.m_mprCursor.l2.P2.X, this.m_mprCursor.l2.P2.Y);
            }

            // paint side marker
            {
                var imgControl = (ImageControl)this.Parent.Parent;
                e.Graphics.DrawString(imgControl.OrientationMarkerLeft, _font, _pen2.Brush, new PointF(this.Width - 20, this.Height / 2));
                e.Graphics.DrawString(imgControl.OrientationMarkerRight, _font, _pen2.Brush, new PointF(0, this.Height / 2));
                e.Graphics.DrawString(imgControl.OrientationMarkerTop, _font, _pen2.Brush, new PointF(this.Width / 2, 0));
                e.Graphics.DrawString(imgControl.OrientationMarkerBottom, _font, _pen2.Brush, new PointF(this.Width / 2, this.Height - 20));
            }
            base.OnPaint(e);
        }

        public float currentZoomFactor { get; set; }

        public float originalZoomFactor { get; set; }

        public RectangleF imageRect { get; set; }

        public int BorderSize { get; set; }

        public Point currentDisplayOffsetPt { get; set; }

        public Point originalDisplayOffsetPt { get; set; }

        public void SetCursorPositionX_Axis(PointF p, Axis axis)
        {
            if (this.m_mprCursor.l1 == null)
            {
                this.m_mprCursor.l1 = new Line();
            }

            PointF p1 = GetActualDisplayPosition(p);
            //p1 = new PointF((float)(this.currentZoomFactor * (p1.X + this.currentDisplayOffsetPt.X)), 
            //               (float)(this.currentZoomFactor * (p1.Y + this.currentDisplayOffsetPt.Y)));
            this.m_mprCursor.l1.P1 = new PointF(p1.X, imageRect.Top);
            this.m_mprCursor.l1.P2 = new PointF(p1.X, imageRect.Bottom);
            this.m_mprCursor.l1.Axis = axis;
            cursorPosition = p1;
        }


        public void SetCursorPositionY_Axis(PointF p, Axis axis)
        {
            if (this.m_mprCursor.l2 == null)
            {
                this.m_mprCursor.l2 = new Line();
            }

           
            PointF p1 = GetActualDisplayPosition(p);
            //p1 = new PointF((float)(this.currentZoomFactor * (p1.X + this.currentDisplayOffsetPt.X)),
            //                (float)(this.currentZoomFactor * (p1.Y + this.currentDisplayOffsetPt.Y)));
            this.m_mprCursor.l2.P1 = new PointF(imageRect.Left, p1.Y);
            this.m_mprCursor.l2.P2 = new PointF(imageRect.Right,p1.Y);
            this.m_mprCursor.l2.Axis = axis;
            cursorPosition = p1;
        }

        public PointF GetActualDisplayPosition(PointF point)
        {
            
           
            PointF p = this.m_coordinateMapping.GetActualDisplayPosition(point);
            p = new PointF((float)(this.currentZoomFactor * (p.X + this.currentDisplayOffsetPt.X)), 
                (float)(this.currentZoomFactor * (p.Y + this.currentDisplayOffsetPt.Y)));
            return p;
        }

        public Point GetOriginalCoords(Point p)
        {
            Point ret = new Point((int)(((p.X / currentZoomFactor) - currentDisplayOffsetPt.X)),
                (int)(((p.Y / currentZoomFactor) - currentDisplayOffsetPt.Y)));
            return this.m_coordinateMapping.GetActualPosition(ret);
        }

        public bool MPRCursorSelected { get; set; }

        public double XPixelSpacing { get; set; }

        public double YPixelSpacing { get; set; }

        public int PixelIntensity { get; set; }

        public bool MPRAxisSelected { get; set; }

        public bool MPRXAxisSelected { get; set; }

        public bool MPRYAxisSelected { get; set; }
    }
}
