using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace MPR_UI
{
    public partial class MPRCursor : Control
    {
        public MPRCursor()
        {
            InitializeComponent();
            SetStyle(ControlStyles.UserPaint, true);
            SetStyle(ControlStyles.AllPaintingInWmPaint, true);
            SetStyle(ControlStyles.DoubleBuffer, true);
            SetStyle(ControlStyles.UserMouse, true);
        }

        public MPRCursor(IContainer container)
        {
            container.Add(this);

            InitializeComponent();
        }
        protected override void OnPaint(PaintEventArgs e)
        {
            base.OnPaint(e);
        }
        protected override void OnMouseClick(MouseEventArgs e)
        {
            MessageBox.Show(this.Parent.Name);
            base.OnMouseClick(e);
        }
        internal void OnMouseDown(MouseEventArgs e)
        {
            base.OnMouseDown(e);
        }

        internal void PaintCursor(PaintEventArgs e)
        {
            OnPaint(e);
        }
    }
}
