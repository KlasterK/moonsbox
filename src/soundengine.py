import pygame
import warnings
import random
from pathlib import Path
import time

from .config import (
    ASSETS_ROOT,
    VOLUME,
)


class _Track:
    def __init__(self, track: pygame.mixer.Sound):
        self.sound = track
        self.begin_time = time.time()
        self.end_time = self.begin_time + self.sound.get_length()

    def is_over(self) -> bool:
        return time.time() > self.end_time


class _SoundEngine:
    def __init__(self):
        self._tracks_cache: dict[str, list[pygame.mixer.Sound]] = {}
        self._current_sounds: dict[str, _Track] = {}
        self._current_categories: dict[str, _Track] = {}

    def _load_tracks(self, sound_name: str) -> None:
        glob_path = Path(ASSETS_ROOT / 'sounds')
        loaded_tracks = []
        for track_path in glob_path.glob(f'{sound_name}.*'):
            track = pygame.mixer.Sound(track_path)
            track.set_volume(VOLUME)
            loaded_tracks.append(track)

        self._tracks_cache[sound_name] = loaded_tracks
        if not loaded_tracks:
            warnings.warn(f'no tracks found for sound {sound_name!r}')

    def _get_track(self, sound_name: str) -> _Track | None:
        tracks = self._tracks_cache.get(sound_name)
        if tracks is None:
            self._load_tracks(sound_name)
            tracks = self._tracks_cache[sound_name]
        if not tracks:
            return None

        sound = random.choice(tracks)
        track = _Track(sound)
        return track

    def play_sound(self, name: str, category: str | None = None, do_override=False) -> None:
        if category is None:
            key = name
            mapping = self._current_sounds
        else:
            key = category
            mapping = self._current_categories

        playing_track = mapping.get(key)

        if playing_track is not None:
            if do_override:
                new_track = self._get_track(name)
                assert new_track is not None

                playing_track.sound.stop()
                mapping[key] = new_track
                mapping[key].sound.play()

            elif playing_track.is_over():
                mapping[key] = None

            else:
                return

        track = self._get_track(name)
        if track is None:
            return
        mapping[key] = track
        track.sound.play()


_sound_engine = _SoundEngine()

play_sound = _sound_engine.play_sound
