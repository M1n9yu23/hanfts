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
plugins {
  alias(libs.plugins.android.application) apply false
  alias(libs.plugins.android.library) apply false
  alias(libs.plugins.android.test) apply false
  alias(libs.plugins.kotlin.compose) apply false
  alias(libs.plugins.spotless)
  alias(libs.plugins.vanniktech.publish) apply false
}

spotless {
  kotlin {
    target("**/*.kt")
    targetExclude("**/build/**/*.kt", "spotless/**")
    ktlint("1.8.0")
    licenseHeaderFile(rootProject.file("spotless/copyright.kt"), "(^(?![\\/ ]))")
  }
  kotlinGradle {
    target("**/*.kts")
    targetExclude("**/build/**/*.kts", "spotless/**")
    ktlint("1.8.0")
    licenseHeaderFile(rootProject.file("spotless/copyright.kt"), "(^(?![\\/ ]))")
  }
  format("cpp") {
    target("**/*.cpp", "**/*.h")
    targetExclude("**/build/**", "**/.cxx/**", "spotless/**")
    licenseHeaderFile(
      rootProject.file("spotless/copyright.cpp"),
      "(#pragma|#include|namespace)",
    )
    trimTrailingWhitespace()
    endWithNewline()
  }
  format("xml") {
    target("**/*.xml")
    targetExclude("**/build/**/*.xml", "**/.cxx/**/*.xml", "spotless/**", "**/.idea/**/*.xml")
    licenseHeaderFile(rootProject.file("spotless/copyright.xml"), "(<[^!?])")
    trimTrailingWhitespace()
    endWithNewline()
  }
}
