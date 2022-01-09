using System;
using System.Collections.Generic;
using System.IO.MemoryMappedFiles;
using System.Text;
using System.Threading;

namespace server
{
    class FileStorage
    {
        private List<object[]> _lstGlobalRef = new List<object[]>();

        public FileStorage()
        {
            //
        }

        public string NamePrefix { get; set; } = string.Empty;

        public int FileCount { get; set; } = 0;

        public long MaxFileSize { get; set; } = 0;

        public void Start()
        {
            long capacity = 4 + (2 * (12 + this.MaxFileSize));
            var n = this.FileCount;
            for (int i = 0; i < n; ++i)
            {
                var name = this.NamePrefix + i.ToString();
                var mutexName = name + ".lck";

                var mutex = new Mutex(false, mutexName);
                var mmf = MemoryMappedFile.CreateNew(name, capacity);
                var view = mmf.CreateViewAccessor(0, capacity);

                _lstGlobalRef.Add(new object[] { mutex, mmf, view });
            }
        }

        //
    }
}
