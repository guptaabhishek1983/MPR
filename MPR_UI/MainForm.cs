using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using ImageUtils;
using MPR_UI.Properties;
using MPR_VTK_BRIDGE;

namespace MPR_UI
{
    [System.Runtime.InteropServices.GuidAttribute("063C04C5-9EA1-45A2-AE52-2AEB854437CE")]
    public partial class MainForm : Form
    {
        private ImageControl axialImage;
        private ImageControl coronalImage;
        private ImageControl sagittalImage;
        public MainForm()
        {
            InitializeComponent();
            this.Load += Form1_Load;
        }

        void Form1_Load(object sender, EventArgs e)
        {
            
            tableLayoutPanel1.SuspendLayout();

            axialImage = new ImageControl(Axis.AxialAxis);
            axialImage.Dock = DockStyle.Fill;
            tableLayoutPanel1.Controls.Add(axialImage, 0, 0);

            sagittalImage = new ImageControl(Axis.SagittalAxis);
            sagittalImage.Dock = DockStyle.Fill;
            tableLayoutPanel1.Controls.Add(sagittalImage, 1, 0);

            coronalImage = new ImageControl(Axis.CoronalAxis);
            coronalImage.Dock = DockStyle.Fill;
            tableLayoutPanel1.Controls.Add(coronalImage, 2, 0);
            
            tableLayoutPanel1.ResumeLayout();
            MessageBox.Show("Done");
        }

        private void MainForm_Resize(object sender, EventArgs e)
        {
            
        }

        private void button1_Click(object sender, EventArgs e)
        {
            button1.Enabled = false;
            MPR_UI_Interface ui_interface = MPR_UI_Interface.GetHandle();
            ui_interface.InitMPR(Settings.Default.DICOM_DIR);
            axialImage.InitScrollBarAndLoadImage();
            sagittalImage.InitScrollBarAndLoadImage();
            coronalImage.InitScrollBarAndLoadImage();
        }
    }
}
