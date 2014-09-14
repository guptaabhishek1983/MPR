using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using MPR_UI.Properties;

namespace MPR_UI
{
    public class Line
    {

        private Axis axis;
        private PointF p1;
        private PointF p2;
        private Pen displayPen;


        public Line()
        {
            axis = Axis.None;
            p1 = new PointF(0, 0);
            p2 = new PointF(0, 0);
            displayPen = new Pen(Color.Red,2.0F);
        }

        public Axis Axis
        {
            get { return axis; }
            set {
                switch (value)
                {
                    case Axis.AxialAxis:
                        displayPen = new Pen(Settings.Default.AxialColor, 2.0F);
                        break;
                    case Axis.CoronalAxis:
                        displayPen = new Pen(Settings.Default.CoronalColor, 2.0F);
                        break;
                    case Axis.SagittalAxis:
                        displayPen = new Pen(Settings.Default.SagittalColor, 2.0F);
                        break;
                    default:
                        displayPen = new Pen(Color.Red,2.0F);
                        break;
                }
                axis = value; 
            }
        }

        public PointF P1
        {
            get { return p1; }
            set { p1 = value; }
        }

        public PointF P2
        {
            get { return p2; }
            set { p2 = value; }
        }
        public Pen DisplayPen
        {
            get { return displayPen; }
        }

    }
}
