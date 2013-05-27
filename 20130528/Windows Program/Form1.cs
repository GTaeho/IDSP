using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using ZedGraph;

namespace RealtimeGraph
{
    public partial class Form1 : Form
    {
        // connection button flag
        private Boolean ConnTgl = false;
        private Boolean SavTgl = false;
        
        // connection and receive thread
        private Thread CNThread;
        private Thread RXThread;

        // crossthreading purpose
        delegate void AddToPlotDelegate(string str);

        // TcpClient address and port
        String addr;
        int port;

        // zedgraph pointpairlist global variable
        PointPairList list1 = new PointPairList();
        PointPairList list2 = new PointPairList();
        PointPairList list3 = new PointPairList();

        // AddToPlot global variable
        double x = 0;
        double EQX, EQY, EQZ;

        // network variables
        private TcpClient client;
        private NetworkStream ns;
        private StreamWriter sw;
        private StreamReader sr;

        // save file stream
        StreamWriter fsw;

        // for better performance
        string[] eachData;

        public Form1()
        {
            InitializeComponent();
        }

        private void CreateGraph(ZedGraphControl zgcX, ZedGraphControl zgcY, ZedGraphControl zgcZ) {

            /*==========   Axis X   ==========*/
            GraphPane EQAxisX = zgcX.GraphPane;

            // set the titles
            EQAxisX.Title.Text = "EW (X축)";
            EQAxisX.XAxis.Title.Text = "Sample Point";
            EQAxisX.YAxis.Title.Text = "Magnitude";

            //EQAxisX.XAxis.Scale.MajorUnit = DateUnit.Minute;
            //EQAxisX.XAxis.Scale.MinorUnit = DateUnit.Second;
            //EQAxisX.XAxis.Scale.Format = "HH:mm:ss";
            EQAxisX.XAxis.Type = AxisType.LinearAsOrdinal;

            EQAxisX.XAxis.Scale.Min = 0;
            EQAxisX.XAxis.Scale.Max = 220;

            EQAxisX.XAxis.MajorGrid.IsVisible = true;
            EQAxisX.YAxis.MajorGrid.IsVisible = true;

            // Add Line to Graph
            LineItem CurveX = EQAxisX.AddCurve("EQ_SAMPLE of X", list1, Color.Green, SymbolType.Plus);
            CurveX.Line.Width = 1.0F;

            // tell zedgraph to refigure the
            // axes since the data changed    
            zgcX.AxisChange();


            /*==========   Axis Y   ==========*/
            GraphPane EQAxisY = zgcY.GraphPane;

            // set the titles
            EQAxisY.Title.Text = "NS (Y축)";
            EQAxisY.XAxis.Title.Text = "Sample Point";
            EQAxisY.YAxis.Title.Text = "Magnitude";

            //EQAxisY.XAxis.Scale.MajorUnit = DateUnit.Minute;
            //EQAxisY.XAxis.Scale.MinorUnit = DateUnit.Second;
            //EQAxisY.XAxis.Scale.Format = "HH:mm:ss";
            EQAxisY.XAxis.Type = AxisType.LinearAsOrdinal;

            EQAxisY.XAxis.Scale.Min = 0;
            EQAxisY.XAxis.Scale.Max = 220;

            EQAxisY.XAxis.MajorGrid.IsVisible = true;
            EQAxisY.YAxis.MajorGrid.IsVisible = true;

            // Add Line to Graph
            LineItem CurveY = EQAxisY.AddCurve("EQ_SAMPLE of Y", list2, Color.HotPink, SymbolType.XCross);
            CurveY.Line.Width = 1.0F;

            // tell zedgraph to refigure the
            // axes since the data changed
            zgcY.AxisChange();

            /*==========   Axis Z   ==========*/
            GraphPane EQAxisZ = zgcZ.GraphPane;

            // set the titles
            EQAxisZ.Title.Text = "UD (Z축)";
            EQAxisZ.XAxis.Title.Text = "Sample Point";
            EQAxisZ.YAxis.Title.Text = "Magnitude";

            //EQAxisZ.XAxis.Scale.MajorUnit = DateUnit.Minute;
            //EQAxisZ.XAxis.Scale.MinorUnit = DateUnit.Second;
            //EQAxisZ.XAxis.Scale.Format = "HH:mm:ss";
            EQAxisZ.XAxis.Type = AxisType.LinearAsOrdinal;

            EQAxisZ.XAxis.Scale.Min = 0;
            EQAxisZ.XAxis.Scale.Max = 220;

            EQAxisZ.XAxis.MajorGrid.IsVisible = true;
            EQAxisZ.YAxis.MajorGrid.IsVisible = true;

            // Add Line to Graph
            LineItem CurveZ = EQAxisZ.AddCurve("EQ_SAMPLE of Z", list3, Color.CadetBlue, SymbolType.Triangle);
            CurveZ.Line.Width = 1.0F;

            // tell zedgraph to refigure the
            // axes since the data changed
            zgcZ.AxisChange();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            CreateGraph(zedGraphControl1, zedGraphControl2, zedGraphControl3);
            // Size the control to fill the form with a margin
            //SetSize();
        }

        private void Form1_Resize(Object sender, EventArgs e)
        {
            SetSize();
        }

        private void SetSize() {
            splitContainer1.Location = new Point(170, 25);
            //leave a small margin around the outside of the control
            splitContainer1.Size = new Size(ClientRectangle.Width - 175, ClientRectangle.Height - 55);

            //pictureBox1.Location = new Point(ClientRectangle.Width - 1000, ClientRectangle.Height - 150);
        }

        private void button1_Click_1(object sender, EventArgs e)
        {
            if(!ConnTgl) {
                ConnTgl = true;

                button1.Text = "접속종료";
                toolStripStatusLabel1.Text = "연결 중입니다.";

                addr = "";

                addr = textBox1.Text + "." + textBox2.Text + "." + textBox3.Text + "." + textBox4.Text;
                port = Int32.Parse(textBox5.Text);

                try {
                    CNThread = new Thread(connect);
                    CNThread.Start();

                } catch(SocketException se) {
                    MessageBox.Show(se.Message.ToString(), "연결오류");
                } catch(InvalidOperationException ioe) {
                    MessageBox.Show(ioe.Message.ToString(), "연결오류");;
                }

            } else if (ConnTgl) {
                ConnTgl = false;

                button1.Text = "접속";

                toolStripStatusLabel1.Text = "연결이 해제되었습니다.";

                if(client != null) {
                    RXThread.Abort();

                    // Close stream
                    sw.Close();
                    sr.Close();
                    client.Close();
                } else {
                    MessageBox.Show("연결 중이거나 연결이 되어있지 않습니다", "연결오류");
                }
            }
        }

        public void connect() {
            try {
                client = new TcpClient(addr, port);

                ns = client.GetStream();
                sw = new StreamWriter(ns);
                sr = new StreamReader(ns);

                // send start transmission signal
                sw.WriteLine("Start");
                sw.Flush();

                toolStripStatusLabel1.Text = "연결되었습니다. 데이터 수신을 시작합니다.";
                label3.Text = "연결되었습니다. 저장 가능합니다.";
                button3.Enabled = true; // enable save button

                RXThread = new Thread(readMsg);
                RXThread.Start();

            } catch(SocketException e) {
                ConnTgl = false;
                button1.Text = "접속";

                toolStripStatusLabel1.Text = "연결에 실패했습니다. 다시 연결해 주세요.";
                MessageBox.Show(e.Message.ToString());
            }
        }

        public void readMsg() {
            try {
                while(client.Connected) {
                    string rmsg = sr.ReadLine();
                    AddToPlot(rmsg);
                }
            } catch(IOException ioe) {
                toolStripStatusLabel1.Text = ioe.Message.ToString();
            }
        }

        private void AddToPlot(string str) {
            eachData = str.Split(new char[] { '_' });

            if (list1.Count > 200) {
                list1.RemoveAt(0);
                list2.RemoveAt(0);
                list3.RemoveAt(0);
            } else {
                EQX = double.Parse(eachData[2]);
                EQY = double.Parse(eachData[3]);
                EQZ = double.Parse(eachData[4]);
                list1.Add(x, EQX);
                list2.Add(x, EQY);
                list3.Add(x, EQZ);
                x++;
            }

            // prevent CrossThread crash from happening
            if (this.zedGraphControl1.InvokeRequired) {
                AddToPlotDelegate addToPlotDelegate = new AddToPlotDelegate(AddToPlot);
                this.zedGraphControl1.Invoke(addToPlotDelegate, str);
            } else {
                // not to overburden CPU
                Thread.Sleep(20);

                //richTextBox1.Text = "EW : " + eachData[0] + "\n" +
                                //"NS : " + eachData[1] + "\n" + "UD : " + eachData[2];

                if (SavTgl) {
                    string mTime = dateTimePicker2.ToString().Substring(44, 19);
                    string mData = " > " + "EW : " + eachData[2] + ", NS : " + eachData[3] + ", UD : " + eachData[4] + "\n";
                    fsw.Write(mTime + mData);
                    fsw.Flush();
                }

                // Draw and refresh for EQX
                zedGraphControl1.AxisChange();
                zedGraphControl1.Invalidate();

                // Draw and refresh for EQY
                zedGraphControl2.AxisChange();
                zedGraphControl2.Invalidate();

                // Draw and refresh for EQZ
                zedGraphControl3.AxisChange();
                zedGraphControl3.Invalidate();
            }
        }       

        private void button3_Click(object sender, EventArgs e) {   
            if(!SavTgl) {
                saveData();
                label3.Text = "저장 중입니다.";
                button3.Text = "중지";
            } else {
                SavTgl = false;
                label3.Text = "저장이 완료되었습니다.";
                button3.Text = "저장";
            }
        }

        private void saveData() {
            DialogResult objDr = saveFileDialog1.ShowDialog();
            if(objDr != DialogResult.Cancel) {
                string FileName = saveFileDialog1.FileName;
                fsw = new StreamWriter(FileName);
                SavTgl = true;  // start recording
            } else {
                // do nothing when user cancelled
            }
        }

        // in case of sudden closure or mistake, close every connection
        private void Form1_FormClosed(object sender, FormClosedEventArgs e) {
            if (client != null) {
                RXThread.Abort();

                // Close stream
                sw.Close();
                sr.Close();
                client.Close();
            } else {
                //MessageBox.Show("연결을 종료하고 프로그램을 종료합니다.", "연결오류");
            }
        }
    }
}

