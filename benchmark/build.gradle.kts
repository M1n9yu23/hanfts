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
  alias(libs.plugins.android.test)
}

android {
  namespace = "com.gyugle.hanfts.benchmark"
  compileSdk = 36

  defaultConfig {
    minSdk = 23
    targetSdk = 36
    testInstrumentationRunner = "androidx.benchmark.junit4.AndroidBenchmarkRunner"
    testInstrumentationRunnerArguments["androidx.benchmark.suppressErrors"] = "EMULATOR,DEBUGGABLE"
  }

  compileOptions {
    sourceCompatibility = JavaVersion.VERSION_21
    targetCompatibility = JavaVersion.VERSION_21
  }

  buildTypes {
    create("benchmark") {
      isDebuggable = false
      signingConfig = signingConfigs.getByName("debug")
      matchingFallbacks += listOf("release")
    }
  }

  targetProjectPath = ":app"
  experimentalProperties["android.experimental.self-instrumenting"] = true
}

kotlin {
  compilerOptions {
    jvmTarget.set(JvmTarget.JVM_21)
  }
}

dependencies {
  implementation(project(":hanfts"))
  implementation(libs.androidx.benchmark.junit4)
  implementation(libs.androidx.test.ext.junit)
}

androidComponents {
  beforeVariants(selector().all()) {
    it.enable = it.buildType == "benchmark"
  }
}
