# Keep the JNI bridge class intact so R8 does not remove or rename
# methods declared as 'external' in Kotlin.
-keep class com.gyugle.hanfts.NativeSearchEngine {
    *;
}
