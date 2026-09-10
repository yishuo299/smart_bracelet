package com.example.test

import android.Manifest
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Icon
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.NavigationBarItemDefaults
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.vectorResource
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import androidx.navigation.NavHostController
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.example.test.data.HealthViewModel
import com.example.test.data.MetricType
import com.example.test.ui.ChartFullScreen
import com.example.test.ui.DebugScreen
import com.example.test.ui.HistoryScreen
import com.example.test.ui.HomeScreen
import com.example.test.ui.MetricDetailScreen
import com.example.test.ui.theme.Background
import com.example.test.ui.theme.CyanPrimary
import com.example.test.ui.theme.TestTheme
import com.example.test.ui.theme.TextDim
import com.example.test.ui.theme.TextPrimary

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            TestTheme {
                HealthApp()
            }
        }
    }
}

@Composable
fun HealthApp() {
    val context = LocalContext.current
    val viewModel: HealthViewModel = viewModel()
    val navController = rememberNavController()
    
    val permissionLauncher = rememberLauncherForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { /* 权限结果，仅 BLUETOOTH_CONNECT 影响设备列表 */ }
    
    LaunchedEffect(Unit) {
        viewModel.initBluetooth(context)
        val permissions = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            arrayOf(Manifest.permission.BLUETOOTH_CONNECT)
        } else {
            arrayOf(Manifest.permission.ACCESS_FINE_LOCATION)
        }
        permissionLauncher.launch(permissions.toSet().toTypedArray())
    }
    
    Scaffold(
        modifier = Modifier.fillMaxSize(),
        bottomBar = {
            BottomNavBar(navController)
        }
    ) { innerPadding ->
        Box(
            modifier = Modifier
                .fillMaxSize()
                .padding(innerPadding)
                .background(Background)
        ) {
            NavHost(
                navController = navController,
                startDestination = "home"
            ) {
                composable("home") {
                    HomeScreen(
                        viewModel = viewModel,
                        onMetricClick = { type ->
                            navController.navigate("detail/${type.name}")
                        },
                        onHistoryClick = { navController.navigate("history") }
                    )
                }
                
                composable("detail/{metricType}") { backStackEntry ->
                    val metricTypeName = backStackEntry.arguments?.getString("metricType") ?: return@composable
                    val metricType = MetricType.valueOf(metricTypeName)
                    
                    MetricDetailScreen(
                        type = metricType,
                        viewModel = viewModel,
                        onBack = { navController.popBackStack() },
                        onChartClick = { navController.navigate("chart/${metricType.name}") }
                    )
                }

                composable("chart/{metricType}") { backStackEntry ->
                    val metricTypeName = backStackEntry.arguments?.getString("metricType") ?: return@composable
                    val metricType = MetricType.valueOf(metricTypeName)

                    ChartFullScreen(
                        type = metricType,
                        viewModel = viewModel,
                        onBack = { navController.popBackStack() }
                    )
                }
                
                composable("debug") {
                    DebugScreen(viewModel = viewModel)
                }
                
                composable("history") {
                    HistoryScreen(
                        viewModel = viewModel,
                        onBack = { navController.popBackStack() }
                    )
                }
            }
        }
    }
}

@Composable
private fun BottomNavBar(navController: NavHostController) {
    val currentRoute = remember { mutableStateOf("home") }
    
    NavigationBar(
        modifier = Modifier
            .fillMaxWidth()
            .height(56.dp),
        containerColor = Background
    ) {
        NavigationBarItem(
            selected = currentRoute.value == "home",
            onClick = {
                currentRoute.value = "home"
                navController.navigate("home") {
                    popUpTo("home") { inclusive = true }
                }
            },
            label = { Text("健康", color = TextPrimary) },
            icon = { Text("❤️") },
            colors = NavigationBarItemDefaults.colors(
                selectedIconColor = CyanPrimary,
                selectedTextColor = CyanPrimary,
                unselectedIconColor = TextDim,
                unselectedTextColor = TextDim,
                indicatorColor = Background
            )
        )
        
        NavigationBarItem(
            selected = currentRoute.value == "debug",
            onClick = {
                currentRoute.value = "debug"
                navController.navigate("debug") { popUpTo("home") }
            },
            label = { Text("调试", color = TextPrimary) },
            icon = { Text("🔧") },
            colors = NavigationBarItemDefaults.colors(
                selectedIconColor = CyanPrimary,
                selectedTextColor = CyanPrimary,
                unselectedIconColor = TextDim,
                unselectedTextColor = TextDim,
                indicatorColor = Background
            )
        )
    }
}
