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
    Document(1L, "오늘산책했다", "저녁을 먹고 동네를 한 바퀴 돌았다. 산책하다 보니 머리가 맑아졌다."),
    Document(2L, "강아지산책", "강아지와 공원을 걸었다. 산책을 마치고 나니 강아지가 금방 잠들었다."),
    Document(3L, "야간산책", "자기 전에 혼자 동네를 걸었다. 산책이 어느새 습관이 됐다."),
    Document(4L, "롤 게임", "솔로랭크 게임을 다섯 판 했다. 연속으로 이겨서 기분이 좋았다."),
    Document(5L, "게임 추천", "새로 나온 RPG 게임을 샀다. 스토리가 탄탄해서 몇 시간이 금방 갔다."),
    Document(6L, "보드게임", "친구들이랑 보드게임을 했다. 생각보다 전략적인 게임이었다."),
    Document(7L, "커피 한 잔", "아침에 커피를 내려 마셨다. 커피 향 덕분에 하루가 기분 좋게 시작됐다."),
    Document(8L, "카페 작업", "카페에서 커피를 시키고 노트북으로 작업했다. 집보다 집중이 잘 됐다."),
    Document(9L, "핸드드립 커피", "새 원두를 사서 핸드드립으로 내렸다. 커피 맛이 생각보다 훨씬 좋았다."),
    Document(10L, "기타 연습", "코드 전환 연습을 한 시간 했다. G코드에서 C코드로 넘어가는 게 아직 버겁다."),
    Document(
      11L,
      "Ranked Game",
      "Played five ranked games today and won most of them. The game feels balanced right now.",
    ),
    Document(
      12L,
      "RPG Night",
      "Started a new RPG game last night. The story is deep and the game world feels huge.",
    ),
    Document(
      13L,
      "Board Game",
      "Played a strategy board game with friends. The game lasted three hours but no one wanted to stop.",
    ),
    Document(
      14L,
      "Morning Coffee",
      "Made espresso at home before work. Strong coffee makes early mornings much easier.",
    ),
    Document(
      15L,
      "Coffee Shop",
      "Worked at a café all afternoon. Good coffee and just enough noise to stay focused.",
    ),
    Document(
      16L,
      "Evening Walk",
      "Walked around the neighbourhood after dinner. The cool night air cleared my head.",
    ),
    Document(
      17L,
      "Guitar Session",
      "Spent an hour on chord transitions. Switching between chords is slowly getting smoother.",
    ),
    Document(
      18L,
      "Jeju Trip",
      "Spent two days on Jeju Island. Walked the Olle trail and ate great seafood.",
    ),
    Document(
      19L,
      "Pasta Night",
      "Made cream pasta from scratch. Cutting back on the cream kept it light.",
    ),
    Document(
      20L,
      "Algorithm Study",
      "Worked through a dynamic programming problem. Breaking it into subproblems was the key.",
    ),
  )
