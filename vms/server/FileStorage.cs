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
        private List<string> _lstFilenamePrefix = new List<string>();

        public FileStorage()
        {
            //
        }

        public long MaxFileSize { get; set; } = 0;

        public void AddFile(string filenamePrefix)
        {
            this._lstFilenamePrefix.Add(filenamePrefix);
        }

        public void Start()
        {
            long capacity = 4 + (2 * (12 + this.MaxFileSize));
            foreach (var name in this._lstFilenamePrefix)
            { 
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
