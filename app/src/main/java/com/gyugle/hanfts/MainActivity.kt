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

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Search
import androidx.compose.material3.Card
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SuggestionChip
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.gyugle.hanfts.Document
import com.gyugle.hanfts.sample.ui.theme.HanftsTheme

class MainActivity : ComponentActivity() {
  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    enableEdgeToEdge()
    setContent {
      HanftsTheme {
        SearchRoute()
      }
    }
  }
}

@Composable
fun SearchRoute(viewModel: SearchViewModel = viewModel()) {
  val query by viewModel.query.collectAsState()
  val hits by viewModel.hits.collectAsState()
  SearchScreen(
    query = query,
    hits = hits,
    documentCount = viewModel.documentCount,
    onQueryChange = viewModel::onQueryChange,
  )
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SearchScreen(
  query: String,
  hits: List<SearchHit>,
  documentCount: Int,
  onQueryChange: (String) -> Unit,
) {
  Scaffold(
    topBar = {
      TopAppBar(
        title = { Text("hanfts") },
        colors =
          TopAppBarDefaults.topAppBarColors(
            containerColor = MaterialTheme.colorScheme.primaryContainer,
            titleContentColor = MaterialTheme.colorScheme.onPrimaryContainer,
          ),
      )
    },
  ) { innerPadding ->
    Column(
      modifier =
        Modifier
          .fillMaxSize()
          .padding(innerPadding),
    ) {
      OutlinedTextField(
        value = query,
        onValueChange = onQueryChange,
        placeholder = { Text("산책, 공원, walk, park…") },
        leadingIcon = { Icon(Icons.Default.Search, contentDescription = null) },
        singleLine = true,
        modifier =
          Modifier
            .fillMaxWidth()
            .padding(horizontal = 16.dp, vertical = 12.dp),
      )

      Text(
        text =
          if (query.isBlank()) {
            "문서 ${documentCount}개 인덱싱됨"
          } else {
            "문서 ${documentCount}개 중 ${hits.size}개 결과"
          },
        style = MaterialTheme.typography.labelMedium,
        color = MaterialTheme.colorScheme.onSurfaceVariant,
        modifier = Modifier.padding(horizontal = 16.dp, vertical = 2.dp),
      )

      if (hits.isEmpty() && query.isNotBlank()) {
        Box(
          modifier = Modifier.fillMaxSize(),
          contentAlignment = Alignment.Center,
        ) {
          Text(
            text = "\"$query\" 에 대한 결과 없음",
            style = MaterialTheme.typography.bodyMedium,
            color = MaterialTheme.colorScheme.onSurfaceVariant,
          )
        }
      } else {
        LazyColumn(
          contentPadding = PaddingValues(horizontal = 16.dp, vertical = 8.dp),
          verticalArrangement = Arrangement.spacedBy(8.dp),
        ) {
          items(hits, key = { it.document.id }) { hit ->
            SearchHitCard(hit)
          }
        }
      }
    }
  }
}

@Composable
private fun SearchHitCard(hit: SearchHit) {
  Card(modifier = Modifier.fillMaxWidth()) {
    Column(modifier = Modifier.padding(16.dp)) {
      Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically,
      ) {
        Text(
          text = hit.document.title,
          style = MaterialTheme.typography.titleMedium,
          modifier = Modifier.weight(1f),
        )
        if (hit.score > 0f) {
          SuggestionChip(
            onClick = {},
            label = { Text("%.2f".format(hit.score)) },
          )
        }
      }
      Text(
        text = hit.document.body,
        style = MaterialTheme.typography.bodyMedium,
        color = MaterialTheme.colorScheme.onSurfaceVariant,
        maxLines = 2,
        overflow = TextOverflow.Ellipsis,
        modifier = Modifier.padding(top = 4.dp),
      )
    }
  }
}

private val previewHits =
  listOf(
    SearchHit(
      Document("walk-cherry-blossom", "벚꽃 산책", "벚꽃이 만개한 길을 걸었다. 꽃잎이 바람에 흩날려 마치 눈처럼 떨어졌다."),
      score = 0f,
    ),
    SearchHit(
      Document(
        "walk-morning",
        "Morning Walk",
        "Woke up early and took a quiet walk through the park. The fresh air was refreshing.",
      ),
      score = 0f,
    ),
    SearchHit(
      Document("walk-hangang", "한강 산책", "한강 공원을 따라 한 시간쯤 걸었다. 강바람이 시원하고 경치가 좋았다."),
      score = 0f,
    ),
  )

@Preview(showSystemUi = true, name = "SearchScreen — browse")
@Composable
private fun SearchScreenBrowsePreview() {
  HanftsTheme {
    SearchScreen(
      query = "",
      hits = previewHits,
      documentCount = 20,
      onQueryChange = {},
    )
  }
}

@Preview(showSystemUi = true, name = "SearchScreen — results")
@Composable
private fun SearchScreenResultsPreview() {
  HanftsTheme {
    SearchScreen(
      query = "산책",
      hits =
        listOf(
          SearchHit(
            Document("walk-cherry-blossom", "벚꽃 산책", "벚꽃이 만개한 길을 걸었다. 꽃잎이 바람에 흩날려 마치 눈처럼 떨어졌다."),
            score = 1.82f,
          ),
          SearchHit(
            Document("walk-hangang", "한강 산책", "한강 공원을 따라 한 시간쯤 걸었다. 강바람이 시원하고 경치가 좋았다."),
            score = 1.21f,
          ),
        ),
      documentCount = 20,
      onQueryChange = {},
    )
  }
}

@Preview(showSystemUi = true, name = "SearchScreen — empty")
@Composable
private fun SearchScreenEmptyPreview() {
  HanftsTheme {
    SearchScreen(
      query = "없는검색어",
      hits = emptyList(),
      documentCount = 20,
      onQueryChange = {},
    )
  }
}
