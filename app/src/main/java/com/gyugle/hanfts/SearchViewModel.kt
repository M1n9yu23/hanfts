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
package com.gyugle.hanfts.sample

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.gyugle.hanfts.Document
import com.gyugle.hanfts.SearchEngine
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

data class SearchHit(val document: Document, val score: Float)

class SearchViewModel : ViewModel() {
  private val engine = SearchEngine()
  private val documentMap = SAMPLE_DOCUMENTS.associateBy { it.id }

  private val _query = MutableStateFlow("")
  val query: StateFlow<String> = _query.asStateFlow()

  private val _hits = MutableStateFlow<List<SearchHit>>(emptyList())
  val hits: StateFlow<List<SearchHit>> = _hits.asStateFlow()

  val documentCount: Int = SAMPLE_DOCUMENTS.size

  private var searchJob: Job? = null

  init {
    viewModelScope.launch(Dispatchers.Default) {
      engine.rebuildIndex(SAMPLE_DOCUMENTS)
      _hits.value = SAMPLE_DOCUMENTS.map { SearchHit(it, 0f) }
    }
  }

  fun onQueryChange(query: String) {
    _query.value = query
    searchJob?.cancel()
    searchJob =
      viewModelScope.launch(Dispatchers.Default) {
        _hits.value =
          if (query.isBlank()) {
            SAMPLE_DOCUMENTS.map { SearchHit(it, 0f) }
          } else {
            engine.search(query).mapNotNull { result ->
              documentMap[result.id]?.let { SearchHit(it, result.score) }
            }
          }
      }
  }

  override fun onCleared() {
    super.onCleared()
    engine.close()
  }
}

private val SAMPLE_DOCUMENTS =
  listOf(
    Document(1L, "오늘 산책", "오늘 오후에 동네 공원을 한 바퀴 걸었다. 날씨가 맑아서 기분이 좋았다."),
    Document(2L, "한강 산책", "한강 공원을 따라 한 시간쯤 걸었다. 강바람이 시원하고 경치가 좋았다."),
    Document(3L, "아침 산책", "이른 아침에 일어나 공원을 천천히 걸었다. 상쾌한 공기 덕에 하루가 개운하게 시작됐다."),
    Document(4L, "저녁 산책", "저녁을 먹고 동네 골목길을 걸었다. 가로등 불빛 아래 걷는 게 생각보다 좋았다."),
    Document(5L, "비 오는 날 산책", "우산을 들고 빗속을 걸었다. 빗소리를 들으며 걷는 것도 나름 좋은 것 같다."),
    Document(6L, "강아지와 산책", "강아지를 데리고 공원을 한 바퀴 돌았다. 강아지가 신이 나서 뛰어다녔다."),
    Document(7L, "가을 단풍 산책", "단풍이 든 길을 걸었다. 빨갛고 노란 낙엽이 바람에 날리는 게 아름다웠다."),
    Document(8L, "눈 오는 날 산책", "첫눈이 내리는 날 산책을 나갔다. 발밑에 눈이 뽀득뽀득 소리를 내며 밟혔다."),
    Document(9L, "벚꽃 산책", "벚꽃이 만개한 길을 걸었다. 꽃잎이 바람에 흩날려 마치 눈처럼 떨어졌다."),
    Document(10L, "산 등산", "주말에 친구와 함께 북한산을 올랐다. 힘들었지만 정상에서 보는 풍경이 멋있었다."),
    Document(
      11L,
      "Morning Walk",
      "Woke up early and took a quiet walk through the park. The fresh air was refreshing.",
    ),
    Document(
      12L,
      "Park Stroll",
      "Spent an hour walking around the local park. The trees were full and the path was peaceful.",
    ),
    Document(
      13L,
      "Evening Walk",
      "After dinner I walked through the neighbourhood. The streetlights made the path feel cozy.",
    ),
    Document(
      14L,
      "Rainy Day Walk",
      "Walked in the light rain with an umbrella. The sound of rain on leaves was surprisingly calming.",
    ),
    Document(
      15L,
      "Walk with My Dog",
      "Took the dog for a long walk in the park. She ran ahead and kept circling back happily.",
    ),
    Document(
      16L,
      "Autumn Leaf Walk",
      "Walked along a trail covered in red and yellow leaves. The crunch underfoot was satisfying.",
    ),
    Document(
      17L,
      "Snowy Morning Walk",
      "First snow of the year today. Every step made a soft crunch and the world felt still and quiet.",
    ),
    Document(
      18L,
      "Cherry Blossom Walk",
      "Walked under cherry blossom trees in full bloom. Petals drifted down like soft pink snow.",
    ),
    Document(
      19L,
      "River Trail Walk",
      "Followed the river trail for about an hour. The water was calm and the breeze was cool.",
    ),
    Document(
      20L,
      "Mountain Hike",
      "Hiked to the summit with a friend on the weekend. Hard going but the view at the top was worth it.",
    ),
  )
