using System;
using System.Runtime.InteropServices;

public class AssetBundleParser
{
#if UNITY_ANDROID || UNITY_EDITOR
	const string LIBNAME = "AssetBundleParser";
#else
	const string LIBNAME = "__Internal";
#endif

	public delegate void MessageHandler(string message);
	public delegate void MergeProgress(string bundlename, int index, int size);

	[DllImport(LIBNAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "set_messagehandler")]
	public static extern void set_messagehandler(MessageHandler callback);

	[DllImport(LIBNAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "assetbundle_check")]
	public static extern bool assetbundle_check([MarshalAs(UnmanagedType.LPStr)]string path);

	[DllImport(LIBNAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "assetbundle_sign")]
	public static extern bool assetbundle_sign([MarshalAs(UnmanagedType.LPStr)]string path);

	[DllImport(LIBNAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "assetbundle_diff_init")]
	public static extern IntPtr assetbundle_diff_init([MarshalAs(UnmanagedType.LPStr)]string patchfile);

	[DllImport(LIBNAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "assetbundle_diff_update")]
	public static extern void assetbundle_diff_update(IntPtr ctx, [MarshalAs(UnmanagedType.LPStr)]string path_from, [MarshalAs(UnmanagedType.LPStr)]string path_to, [MarshalAs(UnmanagedType.LPStr)]string bundlename);

	[DllImport(LIBNAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "assetbundle_diff_final")]
	public static extern int assetbundle_diff_final(IntPtr ctx);

	[DllImport(LIBNAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "assetbundle_merge")]
	public static extern void assetbundle_merge([MarshalAs(UnmanagedType.LPStr)]string patch, [MarshalAs(UnmanagedType.LPStr)]string searchpath, [MarshalAs(UnmanagedType.LPStr)]string newpath, MergeProgress progress);
}