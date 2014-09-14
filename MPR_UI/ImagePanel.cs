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

namespace MPR_UI
{
    public partial class ImageControlImagePanel : Control
    {

        private Bitmap _storedBitmap;
        public Bitmap StoreBitmap
        {
            set
            {
            if (_storedBitmap == null)
            {
                _storedBitmap = value;
                InitializeZoomAndPosition();
            }
            _storedBitmap = value;
            Invalidate();
        }
            get { return _storedBitmap; }
        }

        //public ImagePanel()
        //{
        //    //InitializeComponent();
        //    currentDisplayOffsetPt = new Point(0, 0);
        //    originalDisplayOffsetPt = new Point(0, 0);
        //    BorderSize = 2;
        //    currentZoomFactor = 1.0F;
        //    originalZoomFactor = 1.0F;
        //    SetStyle(ControlStyles.UserPaint, true);
        //    SetStyle(ControlStyles.AllPaintingInWmPaint, true);
        //    SetStyle(ControlStyles.DoubleBuffer, true);
        //    SetStyle(ControlStyles.UserMouse, true);
        //    this.Resize += new EventHandler(ImagePanel_Resize);
        //}

        private void ImagePanel_Resize(object sender, EventArgs e)
        {
            InitializeZoomAndPosition();
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

        protected override void OnMouseDown(MouseEventArgs e)
        {
            base.OnMouseDown(e);
            MessageBox.Show("Clicked!!");
        }
        protected override void OnPaint(PaintEventArgs e)
        {
            if (StoreBitmap == null) return;
            RectangleF srcRect = new RectangleF((e.ClipRectangle.X / currentZoomFactor) - currentDisplayOffsetPt.X,
                    (e.ClipRectangle.Y / currentZoomFactor) - currentDisplayOffsetPt.Y,
                    e.ClipRectangle.Width / currentZoomFactor, e.ClipRectangle.Height / currentZoomFactor);

            Rectangle roundedRectangle = Rectangle.Round(imageRect);
            e.Graphics.DrawRectangle(new Pen(Color.FromArgb(25, 25, 25)), roundedRectangle);
            e.Graphics.InterpolationMode = InterpolationMode.HighQualityBicubic;
            e.Graphics.SmoothingMode = SmoothingMode.AntiAlias;
            e.Graphics.DrawImage(StoreBitmap, e.ClipRectangle, srcRect, GraphicsUnit.Pixel);

            base.OnPaint(e);
        }

        public float currentZoomFactor { get; set; }

        public float originalZoomFactor { get; set; }

        public RectangleF imageRect { get; set; }

        public int BorderSize { get; set; }

        public Point currentDisplayOffsetPt { get; set; }

        public Point originalDisplayOffsetPt { get; set; }
    }
}
