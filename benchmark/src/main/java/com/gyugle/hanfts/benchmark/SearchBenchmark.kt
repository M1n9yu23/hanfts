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
package com.gyugle.hanfts.benchmark

import androidx.benchmark.junit4.BenchmarkRule
import androidx.benchmark.junit4.measureRepeated
import androidx.test.ext.junit.runners.AndroidJUnit4
import com.gyugle.hanfts.Document
import com.gyugle.hanfts.SearchEngine
import org.junit.After
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class SearchBenchmark {
  @get:Rule
  val benchmarkRule = BenchmarkRule()

  private lateinit var engine: SearchEngine

  @Before
  fun setup() {
    engine = SearchEngine()
    engine.rebuildIndex(docs10k)
  }

  @After
  fun teardown() {
    engine.close()
  }

  @Test
  fun rebuildIndex_1k() =
    benchmarkRule.measureRepeated {
      engine.rebuildIndex(docs1k)
    }

  @Test
  fun rebuildIndex_10k() =
    benchmarkRule.measureRepeated {
      engine.rebuildIndex(docs10k)
    }

  @Test
  fun search_korean() =
    benchmarkRule.measureRepeated {
      engine.search("산책")
    }

  @Test
  fun search_english() =
    benchmarkRule.measureRepeated {
      engine.search("morning")
    }

  companion object {
    private val docs1k =
      List(1_000) { i ->
        Document(
          id = i.toLong(),
          title = "오늘의 산책 $i",
          body = "날씨가 맑아서 공원을 걸었다 morning walk $i",
        )
      }

    private val docs10k =
      List(10_000) { i ->
        Document(
          id = i.toLong(),
          title = "오늘의 산책 $i",
          body = "날씨가 맑아서 공원을 걸었다 morning walk $i",
        )
      }
  }
}
