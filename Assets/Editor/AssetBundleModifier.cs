using UnityEngine;
using UnityEditor;
using System.IO;
using System.Collections.Generic;

public class AssetBundleModifier : EditorWindow
{
	[MenuItem("工具/AssetBundle/补丁", false)]
	static public void OpenDPTool()
	{
		AssetBundleModifier modifier = EditorWindow.GetWindow<AssetBundleModifier>(false, "补丁工具", true) as AssetBundleModifier;
		if (modifier != null)
		{
			modifier.Show();
		}
	}

	private void OnEnable()
	{
		AssetBundleParser.set_messagehandler(delegate (string message)
		{
			Debug.LogWarning(message);
		});
	}

	private void OnDisable()
	{
		AssetBundleParser.set_messagehandler(null);
	}

	static void AsignBatch(string path)
	{
		List<string> files = new List<string>();
		SelectionTools.ParseSelectionHandler handler = delegate (string file, string ext)
		{
			files.Add(file);
		};
		SelectionTools.GetSelectionFileList(path, handler);

		int count = files.Count;
		for(int index = 0; index < count; index ++)
		{
			string file = files[index];
			EditorUtility.DisplayProgressBar("正在更新签名", file, (float)index / count);
			if(AssetBundleParser.assetbundle_sign(file) == false)
			{
				Debug.LogFormat("{0}更新签名失败。", file);
			}
		}
		EditorUtility.ClearProgressBar();
	}

	static void CompareAndBuildPatch(string from, string to, string patchs)
	{
		Dictionary<string, bool> abdicts = new Dictionary<string, bool>();
		SelectionTools.ParseSelectionHandler handler = delegate (string file, string ext)
		{
			if(AssetBundleParser.assetbundle_check(file))
			{
				abdicts[SelectionTools.SpliteFileNameWithExt(file)] = true;
			}			
		};
		SelectionTools.GetSelectionFileList(from, handler, "读取之前版本AssetBundle文件列表");
		SelectionTools.GetSelectionFileList(to, handler, "读取新版本AssetBundle文件列表");

		SelectionTools.ParseSelectionHandler handler1 = delegate (string file, string ext)
		{
			File.Delete(file);
		};
		SelectionTools.GetSelectionFileList(patchs, handler1, "清理旧补丁文件");

		int index = 0;
		int count = abdicts.Count;
		System.IntPtr ctx = AssetBundleParser.assetbundle_diff_init(string.Format("{0}/bundle.patch", patchs));
		Dictionary<string, bool> .Enumerator em = abdicts.GetEnumerator();
		while(em.MoveNext())
		{
			string bundlename = em.Current.Key;
			EditorUtility.DisplayProgressBar("正在生成补丁", bundlename, (float)index++ / count);
			AssetBundleParser.assetbundle_diff_update(ctx, string.Format("{0}/{1}", from, bundlename), string.Format("{0}/{1}", to, bundlename), bundlename);
		}
		AssetBundleParser.assetbundle_diff_final(ctx);
		EditorUtility.ClearProgressBar();
	}

	void MergeFromPatch(string from, string to, string patchs)
	{
		List<string> files = new List<string>();
		Dictionary<string, bool> abdicts = new Dictionary<string, bool>();
		SelectionTools.ParseSelectionHandler handler = delegate (string file, string ext)
		{
			if(ext == "patch")
			{
				files.Add(file);				
			}
			abdicts[SelectionTools.SpliteFilePath(file)] = true;
		};
		SelectionTools.GetSelectionFileList(patchs, handler, "正在遍历补丁文件列表");

		int count = files.Count;
		AssetBundleParser.MergeProgress progress = delegate (string bundlename, int index, int size)
		{
			EditorUtility.DisplayProgressBar("正在应用补丁", bundlename, (float)index / size);
		};

		for (int index = 0; index < count; index ++)
		{
			string patchfile = files[index];
			AssetBundleParser.assetbundle_merge(patchfile, from, to, progress);
		}
		EditorUtility.ClearProgressBar();
	}

	void DrawAsign()
	{
		GUILayout.BeginHorizontal();
		GUILayout.Label(_assetbundle_cur);
		if (GUILayout.Button("浏览", GUILayout.Width(80)))
		{
			_assetbundle_cur = EditorUtility.OpenFolderPanel("AssetBundle目录", _assetbundle_prev, null);
		}
		GUILayout.EndHorizontal();

		GUILayout.Space(5);
		if (GUILayout.Button("更新"))
		{
			AsignBatch(_assetbundle_cur);
		}
	}

	void DrawBuildPatch()
	{
		GUILayout.BeginHorizontal();
		GUILayout.Label("From:" + _assetbundle_prev);
		if (GUILayout.Button("浏览", GUILayout.Width(80)))
		{
			_assetbundle_prev = EditorUtility.OpenFolderPanel("AssetBundle目录", _assetbundle_prev, null);
		}
		GUILayout.EndHorizontal();

		GUILayout.BeginHorizontal();
		GUILayout.Label("To:" + _assetbundle_cur);
		if (GUILayout.Button("浏览", GUILayout.Width(80)))
		{
			_assetbundle_cur = EditorUtility.OpenFolderPanel("AssetBundle目录", _assetbundle_cur, null);
		}
		GUILayout.EndHorizontal();

		GUILayout.BeginHorizontal();
		GUILayout.Label("Patch:" + _assetbundle_patch);
		if (GUILayout.Button("浏览", GUILayout.Width(80)))
		{
			_assetbundle_patch = EditorUtility.OpenFolderPanel("生成的补丁目录，该目录会被清空", _assetbundle_patch, null);
		}
		GUILayout.EndHorizontal();

		GUILayout.Space(5);
		if (Directory.Exists(_assetbundle_prev) && Directory.Exists(_assetbundle_cur) && Directory.Exists(_assetbundle_patch))
		{
			if (GUILayout.Button("生成补丁"))
			{
				CompareAndBuildPatch(_assetbundle_prev, _assetbundle_cur, _assetbundle_patch);
			}
		}
	}

	void DrawMerge()
	{
		GUILayout.BeginHorizontal();
		GUILayout.Label("From:" + _assetbundle_prev);
		if (GUILayout.Button("浏览", GUILayout.Width(80)))
		{
			_assetbundle_prev = EditorUtility.OpenFolderPanel("AssetBundle目录", _assetbundle_prev, null);
		}
		GUILayout.EndHorizontal();

		GUILayout.BeginHorizontal();
		GUILayout.Label("To:" + _assetbundle_merged);
		if (GUILayout.Button("浏览", GUILayout.Width(80)))
		{
			_assetbundle_merged = EditorUtility.OpenFolderPanel("存放合并后的AssetBundle", _assetbundle_merged, null);
		}
		GUILayout.EndHorizontal();

		GUILayout.BeginHorizontal();
		GUILayout.Label("Patch:" + _assetbundle_patch);
		if (GUILayout.Button("浏览", GUILayout.Width(80)))
		{
			_assetbundle_patch = EditorUtility.OpenFolderPanel("补丁所在目录", _assetbundle_patch, null);
		}
		GUILayout.EndHorizontal();

		GUILayout.Space(5);
		if (Directory.Exists(_assetbundle_prev) && Directory.Exists(_assetbundle_merged) && Directory.Exists(_assetbundle_patch))
		{
			if (GUILayout.Button("合并"))
			{
				MergeFromPatch(_assetbundle_prev, _assetbundle_merged, _assetbundle_patch);
			}
		}
	}

	void OnGUI()
	{
		Color color = GUI.color;
		Color backgroundColor = GUI.backgroundColor;

		string[] texts = { "更新签名", "生成补丁", "测试合并" };
		GUILayout.BeginHorizontal();
		for(int i = 0; i < 3; i ++)
		{
			if(_workType == i)
			{
				GUI.color = Color.yellow;  //按钮文字颜色
				GUI.backgroundColor = Color.red; //按钮背景颜色
			}
			else
			{
				GUI.color = color;
				GUI.backgroundColor = backgroundColor;
			}

			if(GUILayout.Button(texts[i]))
			{
				_workType = i;
			}
		}
		GUILayout.EndHorizontal();

		GUI.color = color;
		GUI.backgroundColor = backgroundColor;

		if(_workType == 0)
		{
			DrawAsign();
		}
		else if(_workType == 1)
		{
			DrawBuildPatch();
		}
		else
		{
			DrawMerge();
		}
	}

	int _workType = 0;
	string _assetbundle_prev;
	string _assetbundle_cur;
	string _assetbundle_patch;
	string _assetbundle_merged;
}