using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace PCIEDriverTest
{
    public partial class DmaTransfer : Form
    {
        public DmaTransfer()
        {
            InitializeComponent();
        }

        public void ShowProgress(double dmaSpeedMB, int dmaCompleted)
        {
            pgbDmaTransfer.Value = dmaCompleted;
            lalDmaTransfer.Text = dmaSpeedMB + "MB/s";
        }
    }
}
