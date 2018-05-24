using UnityEngine;
using UnityEditor;
using System.IO;
using System.Collections.Generic;

public class SelectionTools
{
	public static void ParseTransform(Transform node, System.Func<Transform, bool> parser)
	{
		if (node == null || parser == null)
		{
			return;
		}

		Transform parent = node.parent;
		if (parser(node) == false)
		{
			return;
		}

		if(node.parent != parent || node.childCount <= 0)
		{
			return;
		}

		Transform[] childs = new Transform[node.childCount];
		for(int i = 0; i < node.childCount; i ++)
		{
			childs[i] = node.GetChild(i);
		}

		for(int i = 0; i < childs.Length; i ++)
		{
			ParseTransform(childs[i], parser);
		}
	}

	static public string GetHierarchy(Transform node, Transform root = null)
	{
		if (node == null || node == root) return "";
		string path = node.name;

		while (node.parent != null)
		{
			node = node.parent;
			if(node == root)
			{
				break;
			}
			path = node.name + "/" + path;
		}
		return path;
	}

	public static string CutPath(string path)
	{
		if(string.IsNullOrEmpty(path) == false)
		{
			path = path.Replace('\\', '/');
			int assetStart = path.IndexOf("Assets/");
			if (assetStart > 0)
			{
				path = path.Substring(assetStart);
			}
		}
		return path;
	}

	static void CollectFiles(string path, SelectionTools.ParseSelectionHandler handler, string prompt)
	{
		if (Directory.Exists(path))
		{
			string assetPath = CutPath(path);
			string[] files = Directory.GetFiles(path, "*.*", SearchOption.AllDirectories);
			for (int i = 0; i < files.Length; i++)
			{
				EditorUtility.DisplayProgressBar(string.IsNullOrEmpty(prompt) ? assetPath : prompt, null, (float)i / files.Length);
				CollectFiles(files[i], handler, prompt);
			}
		}
		else if (File.Exists(path))
		{
			path = CutPath(path);
			handler(path, SpliteFileExt(path));
		}
	}

	public delegate void ParseSelectionHandler(string path, string ext);
	public static void GetSelectionFileList(ParseSelectionHandler handler, string prompt = null)
	{
		if (handler == null)
		{
			return;
		}

		Object[] objects = Selection.objects;
		if (objects == null)
		{
			Debug.Log("没有选中任何文件");
			return;
		}

		List<string> selectFiles = new List<string>();
		for (int i = 0; i < objects.Length; i++)
		{
			string path = AssetDatabase.GetAssetPath(objects[i]);
			if (string.IsNullOrEmpty(path))
			{
				continue;
			}
			CollectFiles(path, handler, prompt);
		}

		EditorUtility.ClearProgressBar();
	}

	public static void GetSelectionFileList(string select, ParseSelectionHandler handler, string prompt = null)
	{
		if (handler == null)
		{
			return;
		}

		if(string.IsNullOrEmpty(select))
		{
			GetSelectionFileList(handler, prompt);
		}
		else
		{
			CollectFiles(select, handler, prompt);
			EditorUtility.ClearProgressBar();
		}
	}

	public static void GetSelectionFileList(List<string> selects, ParseSelectionHandler handler, string prompt = null)
	{
		if (handler == null || selects == null || selects.Count <= 0)
		{
			return;
		}

		for (int i = 0; i < selects.Count; i++)
		{
			string path = selects[i];
			if (string.IsNullOrEmpty(path))
			{
				continue;
			}
			CollectFiles(path, handler, prompt);
		}

		EditorUtility.ClearProgressBar();
	}

	static public string SpliteFileExt(string path)
	{
		if (string.IsNullOrEmpty(path))
		{
			return "";
		}
		string extName = "";
		int extStart = path.LastIndexOf('.');
		if (extStart > 0)
		{
			extName = path.Substring(extStart + 1).Trim().ToLower();
		}
		return extName;
	}

	static public string SpliteFileNameWithExt(string path)
	{
		if (string.IsNullOrEmpty(path))
		{
			return "";
		}
		string fileName = path;
		int nameStart = path.LastIndexOf('/');
		if (nameStart > 0)
		{
			fileName = path.Substring(nameStart + 1).Trim();
		}
		return fileName;
	}

	static public string SpliteFileName(string path)
	{
		path = SpliteFileNameWithExt(path);
		if(string.IsNullOrEmpty(path) == false)
		{
			int extStart = path.LastIndexOf('.');
			if(extStart > 0)
			{
				path = path.Substring(0, extStart);
			}
		}
		return path;
	}

	static public string SpliteFilePath(string path)
	{
		string fileName = path;
		if (string.IsNullOrEmpty(path) == false)
		{
			int nameStart = path.LastIndexOf('/');
			if (nameStart > 0)
			{
				fileName = path.Substring(0, nameStart).Trim();
			}
		}
		return fileName;
	}

	static public string TransformSize(long size)
    {
        float unitNum = 1;
        string unitName = "K";
        if (size >= 1024 * 1024)
        {
            unitName = "M";
            unitNum = size / (1024 * 1024);
        }
        else
        {
            unitNum = size / 1024;
        }
        return string.Format("{0}{1}", unitNum, unitName);
    }
}