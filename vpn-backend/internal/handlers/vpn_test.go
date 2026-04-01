package handlers

import "testing"

func TestAggregateVIPAdBlockStateMixedSourcesKeepsApplied(t *testing.T) {
	configs := []map[string]interface{}{
		{
			"vip_ad_block_applied":    true,
			"vip_ad_block_status":     vipAdBlockStatusApplied,
			"vip_ad_block_dns_source": piHoleDNSSourceHost,
		},
		{
			"vip_ad_block_applied":    true,
			"vip_ad_block_status":     vipAdBlockStatusApplied,
			"vip_ad_block_dns_source": piHoleDNSSourceDocker,
		},
	}

	applied, status, source := aggregateVIPAdBlockState(true, configs)
	if !applied {
		t.Fatalf("expected applied=true for mixed pihole sources")
	}
	if status != vipAdBlockStatusApplied {
		t.Fatalf("expected status=%q, got %q", vipAdBlockStatusApplied, status)
	}
	if source == "" || source == piHoleDNSSourceClean {
		t.Fatalf("expected non-clean source for applied mixed setup, got %q", source)
	}
}

func TestAggregateVIPAdBlockStateRejectsAppliedCleanCombination(t *testing.T) {
	configs := []map[string]interface{}{
		{
			"vip_ad_block_applied":    true,
			"vip_ad_block_status":     vipAdBlockStatusApplied,
			"vip_ad_block_dns_source": piHoleDNSSourceClean,
		},
	}

	applied, status, source := aggregateVIPAdBlockState(true, configs)
	if !applied {
		t.Fatalf("expected applied flag to reflect underlying config record")
	}
	if status != vipAdBlockStatusDegraded {
		t.Fatalf("expected status=%q for applied+clean inconsistency, got %q", vipAdBlockStatusDegraded, status)
	}
	if source != piHoleDNSSourceClean {
		t.Fatalf("expected clean source in degraded state, got %q", source)
	}
}

func TestAggregateVIPAdBlockStateUnavailableWhenNotRequested(t *testing.T) {
	applied, status, source := aggregateVIPAdBlockState(false, []map[string]interface{}{
		{
			"vip_ad_block_applied":    true,
			"vip_ad_block_status":     vipAdBlockStatusApplied,
			"vip_ad_block_dns_source": piHoleDNSSourceHost,
		},
	})

	if applied {
		t.Fatalf("expected applied=false when feature is not requested")
	}
	if status != vipAdBlockStatusUnavailable {
		t.Fatalf("expected status=%q, got %q", vipAdBlockStatusUnavailable, status)
	}
	if source != piHoleDNSSourceClean {
		t.Fatalf("expected clean source, got %q", source)
	}
}
