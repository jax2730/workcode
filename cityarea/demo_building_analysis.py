#!/usr/bin/env python3
"""
Demo script: Building data query and visualization

This script demonstrates:
1. Querying building data from OSM
2. Processing and classifying buildings
3. Generating statistics and visualization

Usage:
    python demo_buildings.py <city_name> [--radius 50000] [--show-stats] [--show-sample]

Examples:
    python demo_buildings.py Beijing --show-stats --show-sample
    python demo_buildings.py Shanghai --radius 60000 --show-sample
"""

import sys
import pathlib
import argparse
from typing import Optional, Dict

# Try to import required packages
try:
    import geopandas as gpd
    import pandas as pd
    import numpy as np
except ImportError:
    print("ERROR: Required packages not found.")
    print("Please install: pip install geopandas pandas numpy")
    sys.exit(1)


class BuildingDataAnalyzer:
    """Analyze building GeoPackage data."""

    def __init__(self, gpkg_path: str):
        """Initialize analyzer with GPKG file.

        Args:
            gpkg_path: Path to All_Building.gpkg file
        """
        self.gpkg_path = pathlib.Path(gpkg_path)
        self.gdf = None
        self.load_data()

    def load_data(self):
        """Load building data from GPKG."""
        if not self.gpkg_path.exists():
            raise FileNotFoundError(f"GPKG file not found: {self.gpkg_path}")

        try:
            self.gdf = gpd.read_file(str(self.gpkg_path), layer='building')
            print(f"✓ Loaded {len(self.gdf)} buildings from {self.gpkg_path.name}")
        except Exception as e:
            raise RuntimeError(f"Failed to load GPKG: {e}")

    def print_statistics(self):
        """Print summary statistics about buildings."""
        if self.gdf is None:
            print("No data loaded")
            return

        print("\n" + "="*60)
        print("BUILDING DATA STATISTICS")
        print("="*60)

        # Overall statistics
        total_buildings = len(self.gdf)
        total_area_km2 = self.gdf['area_m2'].sum() / 1e6
        avg_area_m2 = self.gdf['area_m2'].mean()
        median_area_m2 = self.gdf['area_m2'].median()

        print(f"\nOverall Statistics:")
        print(f"  Total buildings:         {total_buildings:,}")
        print(f"  Total area:              {total_area_km2:,.2f} km²")
        print(f"  Average area:            {avg_area_m2:,.1f} m²")
        print(f"  Median area:             {median_area_m2:,.1f} m²")
        print(f"  Min area:                {self.gdf['area_m2'].min():,.1f} m²")
        print(f"  Max area:                {self.gdf['area_m2'].max():,.1f} m²")

        # Buildings with multiple levels
        has_levels = self.gdf['building:levels'].notna().sum()
        print(f"  Buildings with levels:   {has_levels:,} ({100*has_levels/total_buildings:.1f}%)")

        # Category breakdown
        print(f"\nBuildings by Category:")
        category_stats = self.gdf.groupby('category').agg({
            'id': 'count',
            'area_m2': ['sum', 'mean']
        }).round(1)
        category_stats.columns = ['Count', 'Total Area m²', 'Avg Area m²']
        category_stats['% of Total'] = 100 * category_stats['Count'] / total_buildings
        category_stats.columns = ['Count', 'Total Area m²', 'Avg Area m²', '% of Total']

        for category in sorted(self.gdf['category'].unique()):
            cat_data = self.gdf[self.gdf['category'] == category]
            count = len(cat_data)
            total_area = cat_data['area_m2'].sum()
            pct = 100 * count / total_buildings
            print(f"  {category:20} {count:6,} buildings ({pct:5.1f}%)  {total_area/1e6:8.2f} km²")

        # Building type breakdown
        print(f"\nTop Building Types:")
        building_types = self.gdf['building'].value_counts().head(15)
        for building_type, count in building_types.items():
            pct = 100 * count / total_buildings
            print(f"  {building_type:25} {count:6,} ({pct:5.1f}%)")

        # Named buildings
        has_name = self.gdf[self.gdf['name:local'].notna()]
        print(f"\nNamed Buildings:")
        print(f"  With Chinese name:       {(self.gdf['name:local'].notna()).sum():,}")
        print(f"  With English name:       {(self.gdf['name:en'].notna()).sum():,}")

        # Amenity breakdown
        has_amenity = self.gdf[self.gdf['amenity'].notna()]
        if len(has_amenity) > 0:
            print(f"\nAmenities (Top 10):")
            amenities = self.gdf['amenity'].value_counts().head(10)
            for amenity, count in amenities.items():
                pct = 100 * count / total_buildings
                print(f"  {amenity:25} {count:6,} ({pct:5.1f}%)")

        print("\n" + "="*60)

    def print_sample(self, n_rows: int = 10):
        """Print sample buildings."""
        if self.gdf is None or len(self.gdf) == 0:
            print("No data to display")
            return

        print(f"\n{'='*60}")
        print(f"SAMPLE BUILDINGS (showing {min(n_rows, len(self.gdf))} of {len(self.gdf)})")
        print(f"{'='*60}\n")

        # Select top buildings by area
        sample = self.gdf.nlargest(n_rows, 'area_m2')[
            ['id', 'building', 'category', 'area_m2', 'building:levels', 'name:local']
        ].copy()

        for idx, (i, row) in enumerate(sample.iterrows(), 1):
            print(f"{idx}. ID: {row['id']}")
            print(f"   Type:     {row['building']}")
            print(f"   Category: {row['category']}")
            print(f"   Area:     {row['area_m2']:,.1f} m²")
            if pd.notna(row['building:levels']):
                print(f"   Levels:   {row['building:levels']}")
            if pd.notna(row['name:local']):
                print(f"   Name:     {row['name:local']}")
            print()

    def get_category_summary(self) -> Dict[str, Dict[str, any]]:
        """Get summary by category."""
        if self.gdf is None or len(self.gdf) == 0:
            return {}

        summary = {}
        for category in self.gdf['category'].unique():
            cat_data = self.gdf[self.gdf['category'] == category]
            summary[category] = {
                'count': len(cat_data),
                'total_area_m2': cat_data['area_m2'].sum(),
                'avg_area_m2': cat_data['area_m2'].mean(),
                'median_area_m2': cat_data['area_m2'].median(),
            }

        return summary


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description='Analyze building GPKG data'
    )
    parser.add_argument('gpkg_path', help='Path to All_Building.gpkg file')
    parser.add_argument('--show-stats', action='store_true',
                        help='Print statistics')
    parser.add_argument('--show-sample', action='store_true',
                        help='Print sample buildings')
    parser.add_argument('--sample-count', type=int, default=10,
                        help='Number of sample buildings to show')

    args = parser.parse_args()

    try:
        analyzer = BuildingDataAnalyzer(args.gpkg_path)

        if args.show_stats:
            analyzer.print_statistics()

        if args.show_sample:
            analyzer.print_sample(args.sample_count)

        if not args.show_stats and not args.show_sample:
            # Default: show both
            analyzer.print_statistics()
            analyzer.print_sample()

    except Exception as e:
        print(f"ERROR: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
