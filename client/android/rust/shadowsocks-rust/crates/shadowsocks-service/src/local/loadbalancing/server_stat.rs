//! Server latency statistic

use std::collections::VecDeque;

/// Interval between each check
pub const DEFAULT_CHECK_INTERVAL_SEC: u64 = 10;
/// Timeout of each check
pub const DEFAULT_CHECK_TIMEOUT_SEC: u64 = 5; // A common connection timeout of 5 seconds.

const MAX_SERVER_RTT: u32 = DEFAULT_CHECK_TIMEOUT_SEC as u32 * 1000;
const MAX_LATENCY_QUEUE_SIZE: usize = 59; // Account for the last 10 minutes.

/// Statistic score
#[derive(Debug, Copy, Clone)]
pub enum Score {
    /// Unified latency
    Latency(u32),
    /// Request error
    Errored,
}

/// Statistic of a remote server
#[derive(Debug)]
pub struct ServerStat {
    /// Median of latency time (in millisec)
    ///
    /// Use median instead of average time,
    /// because probing result may have some really bad cases
    rtt: u32,
    /// Total_Fail / Total_Probe
    fail_rate: f64,
    /// Recently probe data
    latency_queue: VecDeque<Score>,
    /// Score's standard deviation
    latency_stdev: f64,
    /// Score's average
    latency_mean: f64,
}

fn max_latency_stdev() -> f64 {
    let mrtt = MAX_SERVER_RTT as f64;
    let avg = (0.0 + mrtt) / 2.0;
    let diff1 = (0.0 - avg) * (0.0 - avg);
    let diff2 = (mrtt - avg) * (mrtt - avg);
    // (1.0 / (2.0 - 1.0)) * (diff1 + diff2).sqrt()
    (diff1 + diff2).sqrt()
}

impl ServerStat {
    pub fn new() -> ServerStat {
        ServerStat {
            rtt: MAX_SERVER_RTT,
            fail_rate: 1.0,
            latency_queue: VecDeque::new(),
            latency_stdev: 0.0,
            latency_mean: 0.0,
        }
    }

    fn score(&self) -> u32 {
        // Normalize rtt
        let nrtt = self.rtt as f64 / MAX_SERVER_RTT as f64;

        // Normalize stdev
        let nstdev = self.latency_stdev / max_latency_stdev();

        const SCORE_RTT_WEIGHT: f64 = 1.0;
        const SCORE_FAIL_WEIGHT: f64 = 3.0;
        const SCORE_STDEV_WEIGHT: f64 = 1.0;

        // Score = (norm_lat * 1.0 + prop_err * 3.0 + stdev * 1.0) / 5.0
        //
        // 1. The lower latency, the better
        // 2. The lower errored count, the better
        // 3. The lower latency's stdev, the better
        let score = (nrtt * SCORE_RTT_WEIGHT + self.fail_rate * SCORE_FAIL_WEIGHT + nstdev * SCORE_STDEV_WEIGHT)
            / (SCORE_RTT_WEIGHT + SCORE_FAIL_WEIGHT + SCORE_STDEV_WEIGHT);

        // Times 10000 converts to u32, for 0.0001 precision
        (score * 10000.0) as u32
    }

    pub fn push_score(&mut self, score: Score) -> u32 {
        self.latency_queue.push_back(score);

        // Only records recently MAX_LATENCY_QUEUE_SIZE probe data
        if self.latency_queue.len() > MAX_LATENCY_QUEUE_SIZE {
            self.latency_queue.pop_front();
        }

        self.recalculate_score()
    }

    fn recalculate_score(&mut self) -> u32 {
        if self.latency_queue.is_empty() {
            return self.score();
        }

        let mut vlat = Vec::with_capacity(self.latency_queue.len());
        let mut cerr = 0;
        for s in &self.latency_queue {
            match *s {
                Score::Errored => cerr += 1,
                Score::Latency(lat) => vlat.push(lat),
            }
        }

        // Error rate
        self.fail_rate = cerr as f64 / self.latency_queue.len() as f64;

        if !vlat.is_empty() {
            vlat.sort();

            // Find median of latency
            let mid = vlat.len() / 2;

            self.rtt = if vlat.len() % 2 == 0 {
                (vlat[mid] + vlat[mid - 1]) / 2
            } else {
                vlat[mid]
            };

            if vlat.len() > 1 {
                // STDEV
                let n = vlat.len() as f64;

                let mut total_lat = 0;
                for s in &vlat {
                    total_lat += *s;
                }
                self.latency_mean = total_lat as f64 / n;
                let mut acc_diff = 0.0;
                for s in &vlat {
                    let diff = *s as f64 - self.latency_mean;
                    acc_diff += diff * diff;
                }
                // Corrected Sample Standard Deviation
                self.latency_stdev = ((1.0 / (n - 1.0)) * acc_diff).sqrt();
            }
        }

        self.score()
    }
}
