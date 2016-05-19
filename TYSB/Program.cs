using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace TYSB
{
    static class Utils
    {
        [DllImport("300Utils.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int OpenPack(string packname);

        [DllImport("300Utils.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void ClosePack(int pack);

        [DllImport("300Utils.dll", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool IsValidPack(int pack);

        [DllImport("300Utils.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern uint GetFileCount(int pack);

        [DllImport("300Utils.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int CreateIterator(int pack);

        [DllImport("300Utils.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void CloseIterator(int iterator);

        [DllImport("300Utils.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int GetPackFromIterator(int iterator);

        [DllImport("300Utils.dll", CallingConvention = CallingConvention.Cdecl)]
        [return : MarshalAs(UnmanagedType.I1)]
        public static extern bool MoveToNextFile(int iterator);

        [DllImport("300Utils.dll", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool SeekIterator(int iterator, uint dest);

        [DllImport("300Utils.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern uint TellIterator(int iterator);

        [DllImport("300Utils.dll", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool IsValidIterator(int iterator);

        [DllImport("300Utils.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern IntPtr GetFileName(int iterator);

        [DllImport("300Utils.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern uint GetFileCompressedSize(int iterator);

        [DllImport("300Utils.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern uint GetFileUncompressedSize(int iterator);

        [DllImport("300Utils.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern ulong GetFileContent(int iterator, [MarshalAs(UnmanagedType.LPArray)] byte[] pBuffer, ulong dwBufferSize);

        public static ulong GetFileContent(int iterator, byte[] buffer)
        {
            if (buffer == null)
                throw new ArgumentNullException(nameof(buffer));

            return GetFileContent(iterator, buffer, (ulong)buffer.Length);
        }
    }

    static class Program
    {
        /// <summary>
        /// 应用程序的主入口点。
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new Form1());
        }
    }
}
