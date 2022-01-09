using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Threading.Tasks;

namespace server
{
    class Recorder
    {
        private string _namePrefix = string.Empty;
        private long _maxMSwapFileSize = 0;
        private string _ffmpeg = string.Empty;
        private string _dataPath = string.Empty;
        private string _fileExt = "mkv";
        private long _minFreeSpace = 0;
        private int _segmentLength = 10;
        private StreamInfo[] _rtspInputs = null;

        public Recorder()
        {
            //
        }

        public string NamePrefix
        {
            get { return this._namePrefix; }
            set { this._namePrefix = value; }
        }

        public string FFmpeg
        {
            get { return this._ffmpeg; }
            set { this._ffmpeg = value; }
        }

        public string DataPath
        {
            get { return this._dataPath; }
            set
            {
                this._dataPath = value.Replace('\\', '/');
                if (!this._dataPath.EndsWith("/", StringComparison.Ordinal))
                {
                    this._dataPath += "/";
                }
            }
        }

        public string FileExtension
        {
            get { return this._fileExt; }
            set
            {
                this._fileExt = value;
                if (!this._fileExt.StartsWith(".", StringComparison.Ordinal))
                {
                    this._fileExt = "." + this._fileExt;
                }
            }
        }

        public long MinimumFreeSpace
        {
            get { return this._minFreeSpace; }
            set { this._minFreeSpace = value; }
        }

        public int SegmentLength
        {
            get { return this._segmentLength; }
            set { this._segmentLength = value; }
        }

        public long MaximumMSwapFileSize
        {
            get { return this._maxMSwapFileSize; }
            set { this._maxMSwapFileSize = value; }
        }

        private class StreamInfo
        {
            public string name = string.Empty;
            public string uri = string.Empty;
        }

        public void InitInputStreams(int count, Dictionary<string, string> cfg)
        {
            this._rtspInputs = new StreamInfo[count];
            for (int i = 0; i < count; ++i)
            {
                StreamInfo si = null;
                var key = "recorder.stream_" + (i + 1).ToString();
                var value = string.Empty;
                if (cfg.TryGetValue(key, out value))
                {
                    var pos = value.IndexOf(':');
                    if (pos > 0)
                    {
                        si = new StreamInfo();
                        si.name = value.Substring(0, pos).Trim();
                        si.uri = value.Substring(pos + 1).Trim();
                    }
                }
                this._rtspInputs[i] = si;
            }
        }

        public void Start()
        {
            int streamIndex = 1;
            foreach (var si in this._rtspInputs)
            {
                if (si != null)
                {
                    _ = this.RunWatchdog(streamIndex, si.name, si.uri);
                }
                ++streamIndex;
            }

            _ = this.RunFreeSpaceWatcher();
        }

        private static string StreamIndexToDirName(int index)
        {
            return index.ToString().PadLeft(4, '0');
        }

        private async Task RunWatchdog(int streamIndex, string name, string uri)
        {
            var dataPath = this._dataPath + StreamIndexToDirName(streamIndex);
            Directory.CreateDirectory(dataPath);

            var sb = new StringBuilder();
            sb.Append("-i ");
            sb.Append(uri);
            sb.Append(" -map 0 -acodec copy -vcodec copy -f tee ");
            sb.Append("\"[f=segment:segment_time=");
            sb.Append(this._segmentLength.ToString());
            sb.Append(":strftime=1]");
            sb.Append(dataPath);
            sb.Append("/");
            sb.Append(name);
            sb.Append("-%Y-%m-%d-%H-%M-%S");
            sb.Append(this._fileExt);
            sb.Append("|[f=segment:segment_time=10]mswpfile://");
            sb.Append(this._maxMSwapFileSize.ToString());
            sb.Append("/");
            sb.Append(this._namePrefix);
            sb.Append(streamIndex.ToString());
            sb.Append("/%d.mkv");
            sb.Append("\"");

            var cmdln = sb.ToString();
            var di = new DriveInfo(dataPath);
            while (true)
            {
                using (var process = new Process())
                {
                    var si = process.StartInfo;
                    si.FileName = this._ffmpeg;
                    si.Arguments = cmdln;
                    si.UseShellExecute = false;
                    si.CreateNoWindow = true;

                    process.Start();
                    while (true)
                    {
                        if (process.HasExited)
                        {
                            break;
                        }
                        await Task.Delay(100);
                    }
                }
                await Task.Delay(100);
            }
        }

        private long FreeUpSpaceForSingleStream(int streamIndex)
        {
            var dataPath = this._dataPath + StreamIndexToDirName(streamIndex);
            var fileList = Directory.GetFiles(dataPath);
            if (fileList.Length == 0)
            {
                return 0;
            }

            Array.Sort(fileList);

            long result = 0;
            var filename = fileList[0];
            try
            {
                var fi = new FileInfo(filename);
                var fileSize = fi.Length;
                File.Delete(filename);
                result = fileSize;
            }
            catch (Exception) { }
            return result;
        }

        private long FreeUpSpace()
        {
            long result = 0;
            var streamCount = this._rtspInputs.Length;
            for (int i = 1; i <= streamCount; ++i)
            {
                var fileSize = FreeUpSpaceForSingleStream(i);
                if (fileSize > 0)
                {
                    result += fileSize;
                }
            }
            return result;
        }

        private async Task RunFreeSpaceWatcher()
        {
            var di = new DriveInfo(this._dataPath);
            while (true)
            {
                var requestSpace = this._minFreeSpace - di.AvailableFreeSpace;
                if (requestSpace > 0)
                {
                    long totalFreeUp = 0;
                    while (totalFreeUp < requestSpace)
                    {
                        var fileSize = this.FreeUpSpace();
                        if (fileSize == 0)
                        {
                            break;
                        }
                        totalFreeUp += fileSize;
                    }
                }
                else
                {
                    await Task.Delay(5000);
                }
            }
        }

        //
    }
}
