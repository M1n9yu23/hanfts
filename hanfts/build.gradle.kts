/*
 * Copyright 2026 Gyugle
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
import org.jetbrains.kotlin.gradle.dsl.JvmTarget

plugins {
  alias(libs.plugins.android.library)
  alias(libs.plugins.vanniktech.publish)
}

android {
  namespace = "com.gyugle.hanfts"
  compileSdk = 36

  defaultConfig {
    minSdk = 21
    consumerProguardFiles("proguard-rules.pro")

    ndk {
      abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86_64")
    }
    externalNativeBuild {
      cmake {
        cppFlags += "-std=c++17"
        arguments += "-DANDROID_STL=c++_shared"
      }
    }
  }

  buildTypes {
    release {
      isMinifyEnabled = false
    }
  }

  compileOptions {
    sourceCompatibility = JavaVersion.VERSION_21
    targetCompatibility = JavaVersion.VERSION_21
  }

  externalNativeBuild {
    cmake {
      path = file("src/main/cpp/CMakeLists.txt")
      version = "3.22.1"
    }
  }
}

kotlin {
  compilerOptions {
    jvmTarget.set(JvmTarget.JVM_21)
  }
}

mavenPublishing {
  publishToMavenCentral()
  signAllPublications()

  coordinates(
    groupId = "io.github.m1n9yu23",
    artifactId = "hanfts",
    version = "1.0.0",
  )

  pom {
    name.set("hanfts")
    description.set(
      "Native Full-Text Search for Android — Korean & English, zero dependencies, pure in-memory.",
    )
    inceptionYear.set("2026")
    url.set("https://github.com/M1n9yu23/hanfts")
    licenses {
      license {
        name.set("Apache License, Version 2.0")
        url.set("https://www.apache.org/licenses/LICENSE-2.0.txt")
        distribution.set("https://www.apache.org/licenses/LICENSE-2.0.txt")
      }
    }
    developers {
      developer {
        id.set("M1n9yu23")
        name.set("MinGyu Son")
        email.set("sonmingyu23@naver.com")
        url.set("https://github.com/M1n9yu23")
      }
    }
    scm {
      url.set("https://github.com/M1n9yu23/hanfts")
      connection.set("scm:git:git://github.com/M1n9yu23/hanfts.git")
      developerConnection.set("scm:git:ssh://git@github.com/M1n9yu23/hanfts.git")
    }
  }
}
