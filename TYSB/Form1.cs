using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace TYSB
{
    public partial class Form1 : Form
    {
        private struct PackFile
        {
            public string FileName { get; }
            public uint CompressSize { get; }
            public uint UncompressSize { get; }

            public uint Index { get; }

            public PackFile(string fileName, uint compressSize, uint uncompressSize, uint index)
            {
                FileName = fileName;
                CompressSize = compressSize;
                UncompressSize = uncompressSize;
                Index = index;
            }
        }

        public Form1()
        {
            InitializeComponent();
        }
        
        private int _pack = -1;
        private async void button1_Click(object sender, EventArgs e)
        {
            if (_pack != -1)
            {
                return;
            }
            
            _pack = Utils.OpenPack(@"D:\Program Files (x86)\《300英雄》\Data.jmp");
            int iterator = Utils.CreateIterator(_pack);
            uint count = Utils.GetFileCount(_pack);
            progressBar1.Value = 0;
            progressBar1.Maximum = (int)count;

            var node = new TreeNode("Data.jmp");
            
            await Task.Run(() =>
            {
                do
                {
                    var packFile = new PackFile(Marshal.PtrToStringAnsi(Utils.GetFileName(iterator)),
                        Utils.GetFileCompressedSize(iterator), Utils.GetFileUncompressedSize(iterator),
                        Utils.TellIterator(iterator));
                    if (progressBar1.Value < progressBar1.Maximum)
                    {
                        progressBar1.Invoke((Action)(() => { ++progressBar1.Value; }));
                    }

                    var dirs = packFile.FileName.Split('\\');
                    var tmpNode = node;
                    foreach (var item in dirs)
                    {
                        tmpNode = tmpNode.Nodes.ContainsKey(item) ? tmpNode.Nodes[item] : tmpNode.Nodes.Add(item);
                        tmpNode.Name = item;
                    }
                    tmpNode.Tag = packFile;
                    tmpNode.ToolTipText = $"压缩大小：{packFile.CompressSize}\n解压大小：{packFile.UncompressSize}";
                } while (Utils.MoveToNextFile(iterator));
            });

            Utils.CloseIterator(iterator);

            treeView1.Nodes.Clear();
            treeView1.Nodes.Add(node);
        }

        private void EnumNode(TreeNode node, ICollection<PackFile> result)
        {
            if (node == null)
            {
                throw new ArgumentNullException(nameof(node));
            }

            if (node.Tag == null)
            {
                foreach (var child in node.Nodes)
                {
                    EnumNode((TreeNode)child, result);
                }
                return;
            }

            result.Add((PackFile) node.Tag);
        }

        private async void ExportFile(TreeNode node)
        {
            if (node == null)
            {
                throw new ArgumentNullException(nameof(node));
            }

            var files = new HashSet<PackFile>();

            EnumNode(node, files);

            progressBar1.Value = 0;
            progressBar1.Maximum = files.Count;

            await Task.Run(() =>
            {
                foreach (var packFile in files)
                {
                    var iter = Utils.CreateIterator(_pack);
                    Utils.SeekIterator(iter, packFile.Index);
                    var buffer = new byte[Utils.GetFileUncompressedSize(iter)];
                    Utils.GetFileContent(iter, buffer);
                    var fileName = packFile.FileName.Substring(packFile.FileName.IndexOf('\\') + 1);
                    Directory.CreateDirectory(fileName.Substring(0, fileName.LastIndexOf('\\')));
                    using (var fs = new FileStream(fileName, FileMode.Create))
                    {
                        fs.Write(buffer, 0, buffer.Length);
                    }

                    if (progressBar1.Value < progressBar1.Maximum)
                    {
                        progressBar1.Invoke((Action) (() => { ++progressBar1.Value; }));
                    }
                }
            });
        }

        private void mnuExport_Click(object sender, EventArgs e)
        {
            var item = treeView1.SelectedNode;
            if (item == null)
                return;

            ExportFile(item);
        }

        private void Form1_FormClosed(object sender, FormClosedEventArgs e)
        {
            if (_pack != -1)
            {
                Utils.ClosePack(_pack);
            }
        }

        private void treeView1_AfterSelect(object sender, TreeViewEventArgs e)
        {
            //contextMenuStrip1.Items[0].Enabled = e.Node?.Tag != null;
        }
    }
}
