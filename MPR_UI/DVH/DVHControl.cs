using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using DevExpress.XtraSplashScreen;
using System.Threading;

using MPR_VTK_BRIDGE;
using MPR_UI.DVH;
namespace MPR_UI
{
    public partial class DVHControl : UserControl
    {
        private MPR_UI_Interface m_UIInterface;
        private Histogram m_histogram;

        private float m_targetPrescribedDose;
        private Dictionary<int, List<double>> m_dvhPoints;
        private Dictionary<int, List<double>> m_computed_dvhPoints;
        public DVHControl()
        {
            InitializeComponent();
            this.m_dvhPoints = new Dictionary<int, List<double> >();
            this.m_computed_dvhPoints = new Dictionary<int, List<double>>();
            this.m_histogram = null;
            this.m_UIInterface = MPR_UI_Interface.GetHandle();
        }

        private void RecreateHistogram()
        {
            if(this.m_histogram!=null)
            {
                this.panel1.Controls.Remove(this.m_histogram);
                this.m_histogram.Dispose();
                this.m_histogram = null;
            }
            if (this.m_histogram == null)
            {
                this.m_histogram = new Histogram();
                this.m_histogram.Dock = DockStyle.Fill;
                this.m_histogram.Parent = this;
                this.panel1.Controls.Add(m_histogram);
            }
        }
        private void generateDVH_BTN_Click(object sender, EventArgs e)
        {
            this.m_UIInterface.GetDVHData(ref this.m_dvhPoints, ref this.m_computed_dvhPoints);
            this.m_targetPrescribedDose = this.m_UIInterface.GetTragetPrescribedDose();

            
            RecreateHistogram();

            ShowVolumeInCC();
        }

        private void ShowVolumeInPercentage()
        {
            RecreateHistogram();
            
            foreach(KeyValuePair<int, List<double> > dvhPoints in m_dvhPoints)
            {
                int roiNumber = dvhPoints.Key;
                double maxVal = dvhPoints.Value.Max();
                var percentVals = dvhPoints.Value.Select(x => x / maxVal).ToList();
                percentVals = percentVals.Select(x => x * 100).ToList();

                this.m_histogram.AddSeries(Convert.ToString(roiNumber), 255, 0,0, percentVals, maxVal, 0,0,0,0, Convert.ToInt16(m_targetPrescribedDose));

            }
            //this.m_histogram.SetUpPercentageGraph();
            Invalidate();

        }

        private void ShowVolumeInCC()
        {
            RecreateHistogram();
            foreach (KeyValuePair<int, List<double>> dvhPoints in m_dvhPoints)
            {
                int roiNumber = dvhPoints.Key;
                double maxVal = dvhPoints.Value.Max();

                this.m_histogram.AddSeries(Convert.ToString(roiNumber), 255, 0, 0, dvhPoints.Value, maxVal, 0, 0, 0, 0, Convert.ToInt16(m_targetPrescribedDose));

                if(roiNumber>2)
                    break;
            }

            foreach (KeyValuePair<int, List<double>> dvhPoints in m_computed_dvhPoints)
            {
               
                int roiNumber = dvhPoints.Key;
                double maxVal = dvhPoints.Value.Max();

                this.m_histogram.AddComputeSeries("Computed_"+Convert.ToString(roiNumber), 255, 0, 0, dvhPoints.Value, maxVal, 0, 0, 0, 0, Convert.ToInt16(m_targetPrescribedDose));

            }
        }

        private void percentView_BTN_Click(object sender, EventArgs e)
        {
            ShowVolumeInPercentage();
        }

        private void ccView_BTN_Click(object sender, EventArgs e)
        {
            ShowVolumeInCC();
        }
       
    }
}
