using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;

namespace server
{
    class Program
    {
        private static string _FFmpeg = string.Empty;
        private static int _MaxInstance = 4;
        private static FileStorage _Storage = null;
        private static Recorder _Recorder = null;


        static void Main(string[] args)
        {
            if (args.Length == 0)
            {
                Console.WriteLine("server <configuration file>");
                return;
            }

            var cfgPath = args[0];
            var cfg = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            LoadConfigurationFile(cfgPath, cfg);

            ApplyConfiguration(cfg);

            _Storage.Start();
            _Recorder.Start();

            while (true)
            {
                Thread.Sleep(1000);
            }
        }

        private static void LoadConfigurationFile(string path, Dictionary<string, string> result)
        {
            var rows = File.ReadAllLines(path);
            foreach (var row in rows)
            {
                if (row.StartsWith("#", StringComparison.Ordinal))
                {
                    continue;
                }

                var pos = row.IndexOf('=');
                if (pos < 0)
                {
                    continue;
                }

                var name = row.Substring(0, pos).Trim();
                var value = row.Substring(pos + 1).Trim();
                result[name] = value;
            }
        }

        private static void ApplyConfiguration(Dictionary<string, string> cfg)
        {
            _FFmpeg = cfg["ffmpeg"];

            var namePrefix = cfg["name_prefix"];
            var maxFileSize = long.Parse(cfg["max_data_size"]);

            _MaxInstance = int.Parse(cfg["max_instance"]);

            _Storage = new FileStorage();
            _Storage.MaxFileSize = maxFileSize;
            for (int i = 1; i <= _MaxInstance; ++i)
            {
                _Storage.AddFile(namePrefix + "f1_" + i.ToString());
            }

            _Recorder = new Recorder();
            _Recorder.FFmpeg = _FFmpeg;
            _Recorder.NamePrefix = namePrefix + "f1_";
            _Recorder.MaximumMSwapFileSize = maxFileSize;
            _Recorder.DataPath = cfg["recorder.data_path"];
            _Recorder.FileExtension = cfg["recorder.file_ext"];
            _Recorder.MinimumFreeSpace = long.Parse(cfg["recorder.min_free_space"]);
            _Recorder.SegmentLength = int.Parse(cfg["recorder.segment_length"]);

            _Recorder.InitInputStreams(_MaxInstance, cfg);
        }


        //
    }
}
