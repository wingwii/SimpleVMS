using System;
using System.Collections.Generic;
using System.IO;

namespace server
{
    class Program
    {
        private static string _FFmpeg = string.Empty;
        private static int _MaxInstance = 4;
        private static FileStorage _Storage = null;


        static void Main(string[] args)
        {
            if (args.Length == 0)
            {
                Console.WriteLine("server <configuration file>");
                return;
            }

            var cfg = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            LoadConfigurationFile(args[0], cfg);

            ApplyConfiguration(cfg);

            _Storage.Start();

            Console.ReadKey();
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
            _MaxInstance = int.Parse(cfg["max_instance"]);

            _Storage = new FileStorage();
            _Storage.FileCount = _MaxInstance;
            _Storage.MaxFileSize = long.Parse(cfg["max_data_size"]);
            _Storage.NamePrefix = namePrefix + "_s1_";

            //
        }


        //
    }
}
